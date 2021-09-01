/*
 * z280dbg.h - simple Z280 tracer
 *
 * Copyright (c) Michal Tomek 2018-2019 <mtdev79b@gmail.com>
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

unsigned int g_quit = 0;
unsigned long long instrcnt = 0;
unsigned long long starttrace = -1LL;

void do_timers();

UINT8 debugger_getmem(device_t *device, offs_t addr) {
	UINT8 *mem = RAMARRAY;
	return mem[addr];
}

void debugger_instruction_hook(device_t *device, offs_t curpc) {
	//printf(".");
	char ibuf[20];
	offs_t dres,i;
	char fbuf[10];
	UINT8 *mem = NULL;
	offs_t transpc;

	instrcnt++;
	do_timers();

	if(VERBOSE) {
		cpu_string_export_z280(device,STATE_GENFLAGS,fbuf);
		printf("%s AF=%04X BC=%04X DE=%04X HL=%04X IX=%04X IY=%04X SSP=%04X USP=%04X MSR=%04X\n",fbuf,
		    cpu_get_state_z280(device,Z280_AF),
			cpu_get_state_z280(device,Z280_BC),
			cpu_get_state_z280(device,Z280_DE),
			cpu_get_state_z280(device,Z280_HL),
			cpu_get_state_z280(device,Z280_IX),
			cpu_get_state_z280(device,Z280_IY),
			cpu_get_state_z280(device,Z280_SSP),
			cpu_get_state_z280(device,Z280_USP),
			cpu_get_state_z280(device,Z280_CR_MSR));
		transpc = curpc;
		cpu_translate_z280(device,AS_PROGRAM,0,&transpc);
		mem = RAMARRAY;
		dres = cpu_disassemble_z280(device,ibuf,transpc,&mem[transpc],0);
		printf("%06X: ",transpc);
		for (i=0;i<(dres &DASMFLAG_LENGTHMASK);i++) printf("%02X",mem[transpc+i]);
		for ( ;i<7;i++) {putchar(' ');putchar(' ');}
		printf(" %s\n",ibuf);
		/*if (strstr(ibuf,",(hl)")||strstr(ibuf," (hl)")||strstr(ibuf,"ldi")||strstr(ibuf,"ldd"))
			printf("\tm:%02X",debugger_getmem(device, cpu_get_state_z280(device,Z280_HL)));
		else if (strstr(ibuf,",(de)"))
			printf("\tm:%02X",debugger_getmem(device, cpu_get_state_z280(device,Z280_DE)));
		else if (strstr(ibuf,",(bc)"))
			printf("\tm:%02X",debugger_getmem(device, cpu_get_state_z280(device,Z280_BC)));
		else if (strstr(ibuf,",(ix"))
			printf("\tm:%02X",debugger_getmem(device, cpu_get_state_z280(device,Z280_IX)+(int8_t)mem[transpc+2]));
		else if (strstr(ibuf,",(iy"))
			printf("\tm:%02X",debugger_getmem(device, cpu_get_state_z280(device,Z280_IY)+(int8_t)mem[transpc+2]));
		else if (strstr(ibuf,"(sp),"))	  // ex (sp),...
			printf("\tm:%02X",debugger_getmem(device, cpu_get_state_z280(device,Z280_SP)));
		putchar('\n');*/
		fflush(stdout);
	}
}
