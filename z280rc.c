/*
 * z280rc.c - Z280RC emulation.
 *
 * Copyright (c) Michal Tomek 2018-2021 <mtdev79b@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#ifdef SOCKETCONSOLE
#define BASE_PORT 10280
#define MAX_SOCKET_PORTS 5
#define XTALCLK 29491200
int enable_quadser = 0;
#include "sconsole.h"
#endif

/*
   The original Z280RC board has IDE wired as little endian whereas Z-BUS is big endian.
   This requires that either the CF image is byte-swapped, or the words are swapped
   in software e.g. after executing an INIRW/OTIRW instruction. 

   To get the behavior of the original board, define IDELE (IDE little endian).

   To run IDE in big endian mode, keep it undefined. This allows accessing disk images
   directly without swapping and is equivalent to twisting the IDE cable, i.e. the
   l.o. IDE byte to AD8-15 and h.o. IDE byte to AD0-7. This is the default for the emulator.
*/
//#define IDELE


#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <fcntl.h>
#define fileno _fileno
#else
#include <signal.h>
#endif

#include "z280/z280.h"
#include "ide/ide.h"
#include "ds1202_1302/ds1202_1302.h"
#include "ins8250/ins8250.h"

UINT8 _ram[2*1048576];

#define RAMARRAY _ram
#include "z280dbg.h"

struct ide_controller *ic0;
FILE* if00;
int ifd00;
struct ide_drive *id00;

uint8_t idemap[16] = {ide_data,0,ide_error_r,0,0,ide_sec_count,0,ide_sec_num,/*ide_altst_r*/
				0,ide_cyl_low,0,ide_cyl_hi,0,ide_dev_head,0,ide_status_r};

#define INS8250_DIVISOR 4 /* ~16 clocks */
unsigned int ins8250_clock = INS8250_DIVISOR;
struct pc16554_device *quadser;

rtc_ds1202_1302_t *rtc;

struct z280_device *cpu;
                       
UINT8 ram_read_byte(offs_t A) {
 	return _ram[A];
}

void ram_write_byte(offs_t A,UINT8 V) {
    _ram[A]=V;
}

UINT16 ram_read_word(offs_t A) {
 	return *(UINT16*)&_ram[A];
}

void ram_write_word(offs_t A,UINT16 V) {
    *(UINT16*)&_ram[A]=V;
}

int console_char_available() {
#ifdef SOCKETCONSOLE
	  return char_available_socket_port(0);
#else
      return _kbhit();
#endif
}

void uart_tx(device_t *device, int channel, UINT8 Value) {
	  //printf("TX: %c", Value);
#ifdef SOCKETCONSOLE
	  tx_socket_port(0, Value);
#else
	  fputc(Value,stdout);
#endif
	  //printf("\n");
}

int uart_rx(device_t *device, int channel) {
	int ioData;
	  //ioData = 0xFF;
	  if(console_char_available()) {
#ifdef SOCKETCONSOLE
	    ioData = rx_socket_port(0);
#else
	    //printf("RX\n");
        ioData = getch();
#endif
		return ioData;
	  }
	return -1;
}

int irq0ackcallback(device_t *device,int irqnum) {
	return 0;
}

int quadser_char_available(int channel) {
#ifdef SOCKETCONSOLE
	  return char_available_socket_port(channel+1);
#else
	return 0;
#endif
}

void quadser_tx(device_t *device, int channel, UINT8 Value) {
#ifdef SOCKETCONSOLE
	  tx_socket_port(channel+1, Value);
#endif
}

int quadser_rx(device_t *device, int channel) {
	int ioData;
	  if(quadser_char_available(channel)) {
#ifdef SOCKETCONSOLE
	    ioData = rx_socket_port(channel+1);
#endif
		return ioData;
	  }
	return -1;
}

void quadser_int_state_cb(device_t *device, int state) {
	/*if (VERBOSE) printf("QUADSER int: %d\n",state);*/
	z280_set_irq_line(cpu,1,state); /* INTB line */
}

UINT8 io_read_byte (offs_t Port) {
	uint8_t ioData = 0;

	offs_t lPort = Port & 0xff;
	// IDE emulation
	if (lPort >= 0xc0 && lPort <= 0xcf) {
		ioData = ide_read16(ic0,idemap[lPort-0xc0]);
	}
	else if (lPort == 0xa2) // RTC
	{
		ioData=ds1202_1302_read_data_line(rtc)<<7;
		if (VERBOSE) printf("RTC read: %02x\n",ioData);
	}
	else if (enable_quadser && lPort >= 0xd0 && lPort <= 0xef) // Quadser
	{
		ioData=pc16554_device_r(quadser,lPort-0xd0);
		/*printf("IO: Quadser read b,%x %02x\n",Port,ioData);*/
	}
	else
	{
		printf("IO: Bogus read b,%x\n",Port);
	}
	return ioData;
}

void io_write_byte (offs_t Port,UINT8 Value) {
	offs_t lPort = Port & 0xff;
	
	// IDE emulation
	if (lPort >= 0xc0 && lPort <= 0xcf) {
		ide_write16(ic0,idemap[lPort-0xc0],Value);
	}
	else if (lPort == 0xa0)
	{
		// unmap CFinit
	}
	else if (lPort == 0xa2) // RTC
	{
		// DS1302
		// b7=IO,b1=/RST,b0=CLK
		if (VERBOSE) printf("RTC write: %02x\n",Value);
		ds1202_1302_set_lines(rtc,(Value&2)>>1,Value&1,Value>>7);
	}
	else if (enable_quadser && lPort >= 0xd0 && lPort <= 0xef) // Quadser
	{
		/*printf("IO: Quadser write b,%x %02x\n",Port,Value);*/
		pc16554_device_w(quadser,lPort-0xd0,Value);
	}
	else
	{
		printf("IO: Bogus write b,%x:%x\n",Port,Value);
	}
}

UINT16 io_read_word (offs_t Port) {
	offs_t lPort = Port & 0xff;
	uint16_t ioData = 0;

	if (lPort >= 0xc0 && lPort <= 0xcf) {
		ioData = ide_read16(ic0,idemap[lPort-0xc0]);
#ifdef IDELE
		ioData = (ioData << 8) | (ioData >> 8);
#endif
	}
	else if (lPort == 0xa2) // RTC
	{
		ioData=ds1202_1302_read_data_line(rtc)<<7;
		if (VERBOSE) printf("RTC read: %04x\n",ioData);
	}
	else
	{
		printf("IO: Bogus read w,%x\n",Port);
	}

	return ioData;
}

void io_write_word (offs_t Port,UINT16 Value) {
	offs_t lPort = Port & 0xff;

	if (lPort >= 0xc0 && lPort <= 0xcf) {
#ifdef IDELE
		Value = (Value << 8) | (Value >> 8);
#endif
		ide_write16(ic0,idemap[lPort-0xc0],Value);
    }
	else if (lPort == 0xa2) // RTC
	{
		// DS1302
		// b7=IO,b1=/RST,b0=CLK
		if (VERBOSE) printf("RTC write: %04x\n",Value);
		ds1202_1302_set_lines(rtc,(Value&2)>>1,Value&1,Value>>7);
	}
	else
	{
		printf("IO: Bogus write w,%x:%x\n",Port,Value);
	}
}

UINT8 init_bti(device_t *device) {
    // DIC: 0
	// BS: 0 CF, 1=UART // the board has a jumper for UART bootstrap. TODO
	// LM: 0 =no wait
	// CS: 0 =1/2 clock
	return 0;
}

void do_timers() {
	if (!--ins8250_clock) {
		ins8250_device_timer(quadser->channel[0]);
		if (enable_quadser > 1)
		{
			ins8250_device_timer(quadser->channel[1]);
			if (enable_quadser == 4)
			{
				ins8250_device_timer(quadser->channel[2]);
				ins8250_device_timer(quadser->channel[3]);
			}
		}
		ins8250_clock = INS8250_DIVISOR;
	}
}

void boot1dma () {
   FILE* f;
   if (!(f=fopen("cfmonldr.bin","rb"))) {
     printf("No ROM found.\n");
	 g_quit = 1;
   } else {
     // CFinit does mmap of boot sector to 0-1ffh
	 // let's just load it there for now
	 // TODO
     fread(&_ram[0],1,256,f);
     fclose(f);
   }
}

void io_device_update() {
#ifdef SOCKETCONSOLE
    // check socket open and optionally reopen it
    if (!is_connected_socket_port(0)) open_socket_port(0);
	if (enable_quadser) {
		if (!is_connected_socket_port(1)) open_socket_port(1);
		if (enable_quadser > 1)
		{
			if (!is_connected_socket_port(2)) open_socket_port(2);
			if (enable_quadser == 4) {
				if (!is_connected_socket_port(3)) open_socket_port(3);
				if (!is_connected_socket_port(4)) open_socket_port(4);
			}
		}
	}
#endif
}

void CloseIDE() {
   ide_free(ic0);
}

void InitIDE() {
   const char *devf = getenv("IDE0");

   if (!devf) {
     devf = "cf00.dsk";
   }
   ic0=ide_allocate("IDE0");
   if ((if00 = fopen(devf, "r+b"))) {
     ifd00 = fileno(if00);
     ide_attach(ic0, 0, ifd00);
   } else {
     printf("New device file %s\n", devf);
   }
   ide_reset_begin(ic0);
   atexit(CloseIDE);
}

#ifndef _WIN32
void sigint_handler(int s)	{
	// POSIX SIGINT handler
	// do nothing
}

void sigquit_handler(int s)	{
	// POSIX SIGQUIT handler
	printf("\nExiting emulation.\n");
	shutdown_socket_ports(); // close sockets to prevent waiting for a connection
	g_quit = 1; // make sure atexit is called
}
#endif

void disableCTRLC() {
#ifdef _WIN32
	HANDLE consoleHandle = GetStdHandle(STD_INPUT_HANDLE);
	DWORD consoleMode;
	GetConsoleMode(consoleHandle,&consoleMode);
	SetConsoleMode(consoleHandle,consoleMode&~ENABLE_PROCESSED_INPUT);
#else
	signal(SIGINT, sigint_handler);
#endif
}

struct address_space ram = {ram_read_byte,ram_read_word,ram_write_byte,ram_write_word,ram_read_byte,ram_read_word};
struct address_space iospace = {io_read_byte,io_read_word,io_write_byte,io_write_word,NULL,NULL};

void destroy_rtc()
{
	ds1202_1302_destroy(rtc,1);
}

int main(int argc, char** argv)
{
	printf("z280emu v1.0 Z280RC\n");

	disableCTRLC();
#ifndef _WIN32
	// on POSIX, route SIGQUIT (CTRL+\) to graceful shutdown
	signal(SIGQUIT, sigquit_handler);
#endif
	// on MINGW, keep CTRL+Break (and window close button) enabled
	// MINGW always calls atexit in these cases

	// parse arguments
	int i;
	for (i = 0; i < argc; i++)
	{
		if (argv[i][0]=='-')
		{
			if (argv[i][1]=='d')
			{
				starttrace = 0;
				if (argv[i][2]=='=')
				{
					starttrace = atoll(&argv[i][3]);
				}
				VERBOSE = starttrace==0?1:0;
			} 
			else if (strncmp(argv[i],"-quadser",8)==0)
			{
				enable_quadser = 1;
				if (argv[i][8]=='=')
				{
					if (argv[i][9]=='2')
						enable_quadser = 2;
					else if (argv[i][9]=='4')
						enable_quadser = 4;
				}
			}
		}
	}

#ifdef SOCKETCONSOLE
	init_TCPIP();
	init_socket_port(0); // UART Console
	if (enable_quadser)
	{
	    init_socket_port(1);
	    if (enable_quadser > 1)
		{
			init_socket_port(2);
			if (enable_quadser == 4) {
				init_socket_port(3);
				init_socket_port(4);
			}
		}
	}
	atexit(shutdown_socket_ports);
#endif
	io_device_update(); // wait for serial socket connections

#ifdef _WIN32
	setmode(fileno(stdout), O_BINARY);
#endif

	boot1dma();
	InitIDE();

	rtc = ds1202_1302_init("RTC",1302);
	ds1202_1302_reset(rtc);
	atexit(destroy_rtc);

	/* cpu is @XTALCLK/2 (14.7456)
	   bus is cpu/2      ( 7.3728)
	   ctin1 is bus/4    ( 1.8432) */
	cpu = cpu_create_z280("Z280",Z280_TYPE_Z280,XTALCLK/2,&ram,&iospace,irq0ackcallback,NULL/*daisychain*/,
		init_bti,1/*Z-BUS*/,0,XTALCLK/16,0,uart_rx,uart_tx);
	cpu_reset_z280(cpu);

	quadser = pc16554_device_create("QUADSER", cpu, cpu->m_clock/2, OX16950,
		quadser_int_state_cb,
		quadser_rx,quadser_tx,0/*CLKSEL=GND*/);

	// DMA2,3 /RDY are tied to GND
	z280_set_rdy_line(cpu, 2, ASSERT_LINE);
	z280_set_rdy_line(cpu, 3, ASSERT_LINE);

	struct timeval t0;
	struct timeval t1;
	gettimeofday(&t0, 0);
	int runtime=50000;

	//g_quit = 0;
	while(!g_quit) {
		if(instrcnt>=starttrace) VERBOSE=1;
		cpu_execute_z280(cpu,10000);
		io_device_update();
		/*if (!(--runtime))
			g_quit=1;*/
	}
	gettimeofday(&t1, 0);
	printf("instrs:%llu, time:%g\n",instrcnt, (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f);

}
