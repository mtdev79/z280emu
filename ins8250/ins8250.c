// license:BSD-3-Clause
// copyright-holders:smf, Carl
/**********************************************************************

    National Semiconductor 8250 UART interface and emulation

   More information on the different models can be found in
   section 1.6 at this location:
     http://www.freebsd.org/doc/en_US.ISO8859-1/articles/serial-uart/

Model overview (from page above):

INS8250
This part was used in the original IBM PC and IBM PC/XT. The original name
for this part was the INS8250 ACE (Asynchronous Communications Element) and
it is made from NMOS technology.

The 8250 uses eight I/O ports and has a one-byte send and a one-byte receive
buffer. This original UART has several race conditions and other flaws. The
original IBM BIOS includes code to work around these flaws, but this made
the BIOS dependent on the flaws being present, so subsequent parts like the
8250A, 16450 or 16550 could not be used in the original IBM PC or IBM PC/XT.

The original 8250 pulses the interrupt line if a higher priority interrupt is
cleared but a lower priority one is still active.  It also clears the tsre bit
for a moment before loading the tsr from the thr.  These may be the bugs the
PC and XT depend on as the 8250A and up fix them.

INS8250-B
This is the slower speed of the INS8250 made from NMOS technology. It contains
the same problems as the original INS8250.

INS8250A
An improved version of the INS8250 using XMOS technology with various functional
flaws corrected. The INS8250A was used initially in PC clone computers by vendors
who used "clean" BIOS designs. Because of the corrections in the chip, this part
could not be used with a BIOS compatible with the INS8250 or INS8250B.

INS82C50A
This is a CMOS version (low power consumption) of the INS8250A and has similar
functional characteristics.

NS16450
Same as NS8250A with improvements so it can be used with faster CPU bus designs.
IBM used this part in the IBM AT and updated the IBM BIOS to no longer rely on
the bugs in the INS8250.

NS16C450
This is a CMOS version (low power consumption) of the NS16450.

NS16550
Same as NS16450 with a 16-byte send and receive buffer but the buffer design
was flawed and could not be reliably be used.

The 16550 sometimes will send more then one character over the bus from the fifo
when the rbr is read making the rx fifo useless.  It's unlikely anything depends
on this behavior.

NS16550A
Same as NS16550 with the buffer flaws corrected. The 16550A and its successors
have become the most popular UART design in the PC industry, mainly due to
its ability to reliably handle higher data rates on operating systems with
sluggish interrupt response times.

NS16C552
This component consists of two NS16C550A CMOS UARTs in a single package.

PC16550D
Same as NS16550A with subtle flaws corrected. This is revision D of the
16550 family and is the latest design available from National Semiconductor.

Intel 82050
Essentially a NS16450 squeezed into a 28-pin package with some minor functions
eliminated. Can be strapped for either a 18.432 MHz XTAL (divided by 10 to
produce the BRG clock) or an externally generated 9.216 or 18.432 MHz clock.

Intel 82510
A functional expansion of the 82050 with dozens of extra registers. Adds 4-byte
Tx/Rx FIFOs with programmable thresholds, MCS-51 compatible 9-bit protocol,
ASCII/EBCDIC control character recognition, timed interrupts and more.


Known issues:
- MESS does currently not handle all these model specific features.


History:
    KT - 14-Jun-2000 - Improved Interrupt setting/clearing
    KT - moved into separate file so it can be used in Super I/O emulation and
        any other system which uses a PC type COM port
    KT - 24-Jun-2000 - removed pc specific input port tests. More compatible
        with PCW16 and PCW16 doesn't requre the PC input port definitions
        which are not required by the PCW16 hardware

**********************************************************************/

#include <stdlib.h>
#include <string.h>
//#include "emu.h"
#include "ins8250.h"

/*DEFINE_DEVICE_TYPE(INS8250,  ins8250_device, "ins8250",  "National Semiconductor INS8250 UART")
DEFINE_DEVICE_TYPE(NS16450,  ns16450_device, "ns16450",  "National Semiconductor NS16450 UART")
DEFINE_DEVICE_TYPE(NS16550,  ns16550_device, "ns16550",  "National Semiconductor NS16550 UART")
DEFINE_DEVICE_TYPE(PC16552D, pc16552_device, "pc16552d", "National Semiconductor PC16552D DUART")
DEFINE_DEVICE_TYPE(PC16554,  pc16554_device, "pc16554",  "16554 QUART")

ins8250_uart_device::ins8250_uart_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock, dev_type device_type)
	: device_t(mconfig, type, tag, owner, clock)
	, device_serial_interface(mconfig, *this)
	, m_device_type(device_type)
	, m_out_tx_cb(*this)
	, m_out_dtr_cb(*this)
	, m_out_rts_cb(*this)
	, m_out_int_cb(*this)
	, m_out_out1_cb(*this)
	, m_out_out2_cb(*this)
	, m_rxd(1)
	, m_dcd(1)
	, m_dsr(1)
	, m_ri(1)
	, m_cts(1)
{
	m_regs.ier = 0;
}*/

struct ins8250_device *ins8250_device_create(char *tag, device_t *owner, uint32_t clock, enum ins8250_dev_type device_type,
	devcb_write_line out_int_cb,
	rx_callback_t rx_callback,tx_callback_t tx_callback, int clksel)
{
	struct ins8250_device *d = malloc(sizeof(struct ins8250_device));
	memset(d,0,sizeof(struct ins8250_device));
	d->m_type = device_type;
	d->m_clock = clock;
	d->m_tag = tag;
	d->m_owner = owner;

	d->tx_callback = tx_callback;
	d->rx_callback = rx_callback;
	d->m_out_int_cb = out_int_cb;
	d->m_clksel = clksel;

	d->m_rxd = 1;
	d->m_dcd = 1;
	d->m_dsr = 1;
	d->m_ri = 1;
	d->m_cts = 1;

	//d->m_regs.ier = 0;
	ins8250_device_reset(d);
	return d;
}

/*ins8250_device::ins8250_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: ins8250_uart_device(mconfig, INS8250, tag, owner, clock, dev_type::INS8250)
{
}

ns16450_device::ns16450_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: ins8250_uart_device(mconfig, NS16450, tag, owner, clock, dev_type::NS16450)
{
}

ns16550_device::ns16550_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: ins8250_uart_device(mconfig, NS16550, tag, owner, clock, dev_type::NS16550)
{
}

pc16552_device::pc16552_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, PC16552D, tag, owner, clock)
{
}*/

struct pc16552_device* pc16552_device_create(char *tag, device_t *owner, uint32_t clock, enum ins8250_dev_type device_type,
	devcb_write_line out_int_cb,
	rx_callback_t rx_callback,tx_callback_t tx_callback, int clksel)
{
	struct pc16552_device *d = malloc(sizeof(struct pc16552_device));
	memset(d,0,sizeof(struct pc16552_device));
	int l = strlen(tag);
	char *t = malloc(l+3);
	strcpy(t,tag);strcat(t,".0");
	d->m_chan0 = ins8250_device_create(t, d, clock, device_type, out_int_cb, rx_callback, tx_callback, clksel);
	t = malloc(l+3);
	strcpy(t,tag);strcat(t,".1");
	d->m_chan1 = ins8250_device_create(t, d, clock, device_type, out_int_cb, rx_callback, tx_callback, clksel);
	return d;
}

struct pc16554_device* pc16554_device_create(char *tag, device_t *owner, uint32_t clock, enum ins8250_dev_type device_type,
	devcb_write_line out_int_cb,
	rx_callback_t rx_callback,tx_callback_t tx_callback, int clksel)
{
	struct pc16554_device *d = malloc(sizeof(struct pc16554_device));
	memset(d,0,sizeof(struct pc16554_device));
	int l = strlen(tag);
	char *t = malloc(l+3);
	strcpy(t,tag);strcat(t,".0");
	d->m_chan0 = ins8250_device_create(t, d, clock, device_type, out_int_cb, rx_callback, tx_callback, clksel);
	t = malloc(l+3);
	strcpy(t,tag);strcat(t,".1");
	d->m_chan1 = ins8250_device_create(t, d, clock, device_type, out_int_cb, rx_callback, tx_callback, clksel);
	t = malloc(l+3);
	strcpy(t,tag);strcat(t,".2");
	d->m_chan2 = ins8250_device_create(t, d, clock, device_type, out_int_cb, rx_callback, tx_callback, clksel);
	t = malloc(l+3);
	strcpy(t,tag);strcat(t,".3");
	d->m_chan3 = ins8250_device_create(t, d, clock, device_type, out_int_cb, rx_callback, tx_callback, clksel);
	return d;
}

/* int's pending */
#define COM_INT_PENDING_RECEIVED_DATA_AVAILABLE 0x0001
#define COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY 0x0002
#define COM_INT_PENDING_RECEIVER_LINE_STATUS 0x0004
#define COM_INT_PENDING_MODEM_STATUS_REGISTER 0x0008
#define COM_INT_PENDING_CHAR_TIMEOUT 0x0011

#define INS8250_LSR_TSRE 0x40
#define INS8250_LSR_THRE 0x20
//#define INS8250_LSR_BI 0x10
//#define INS8250_LSR_FE 0x08
//#define INS8250_LSR_PE 0x04
#define INS8250_LSR_OE 0x02
#define INS8250_LSR_DR 0x01

#define INS8250_MCR_DTR 0x01
#define INS8250_MCR_RTS 0x02
#define INS8250_MCR_OUT1 0x04
#define INS8250_MCR_OUT2 0x08
#define INS8250_MCR_LOOPBACK 0x10

#define INS8250_LCR_BITCOUNT_MASK 0x03
#define INS8250_LCR_2STOP_BITS 0x04
//#define INS8250_LCR_PEN 0x08
//#define INS8250_LCR_EVEN_PAR 0x10
//#define INS8250_LCR_PARITY 0x20
//#define INS8250_LCR_BREAK 0x40
#define INS8250_LCR_DLAB 0x80

#define receive_register_reset(ch) ch->rx_bits_rem = 0
#define transmit_register_reset(ch) ch->tx_bits_rem = 0
#define set_tra_rate(ch,x) /**/
#define set_rcv_rate(ch,x) /**/
#define is_transmit_register_empty(ch) (ch->tx_bits_rem == 0)
#define transmit_register_setup(ch,data) ch->tx_data = data; ch->tx_bits_rem = ch->m_bit_count	/* load character into shift register */
#define transmit_register_get_data_bit(ch) 0
#define receive_register_extract(ch) /**/
#define get_received_char(ch) ch->rx_data

//void ins8250_device_rcv_complete(struct ins8250_device *d);
void ins8250_device_tra_complete(struct ins8250_device *d);
void ins8250_device_update_msr(struct ins8250_device *d);
void ns16550_device_push_tx(struct ins8250_device *d, uint8_t data);
uint8_t ns16550_device_pop_rx(struct ins8250_device *d);
void ns16550_device_set_fcr(struct ins8250_device *d, uint8_t data);
void ns16550_device_set_timer(struct ins8250_device *d);

/* ints will continue to be set for as long as there are ints pending */
void ins8250_device_update_interrupt(struct ins8250_device *d)
{
	int state;

	/* if any bits are set and are enabled */
	if (((d->m_int_pending & d->m_regs.ier) & 0x0f) != 0)
	{
		/* trigger next highest priority int */

		/* set int */
		state = 1;
		d->m_regs.iir &= ~(0x08|0x04|0x02);

		/* highest to lowest */
		if (d->m_regs.ier & d->m_int_pending & COM_INT_PENDING_RECEIVER_LINE_STATUS)
			d->m_regs.iir |=0x04|0x02;
		else if (d->m_regs.ier & d->m_int_pending & COM_INT_PENDING_RECEIVED_DATA_AVAILABLE)
		{
			d->m_regs.iir |=0x04;
			if ((d->m_int_pending & COM_INT_PENDING_CHAR_TIMEOUT) == 0x11)
				d->m_regs.iir |= 0x08;
		}
		else if (d->m_regs.ier & d->m_int_pending & COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY)
			d->m_regs.iir |=0x02;

		/* int pending */
		d->m_regs.iir &= ~0x01;
	}
	else
	{
		/* clear int */
		state = 0;

		/* no ints pending */
		d->m_regs.iir |= 0x01;
		/* priority level */
		d->m_regs.iir &= ~(0x08|0x04|0x02);
	}

	LOG("8250 [%s] set int line = %d\n",d->m_tag,state);
	/* set or clear the int */
	if(d->m_out_int_cb)
		d->m_out_int_cb(d,state);
}

/* set pending bit and trigger int */
void ins8250_device_trigger_int(struct ins8250_device *d, int flag)
{
	d->m_int_pending |= flag;
	ins8250_device_update_interrupt(d);
	LOG("8250 [%s] set int source=%d\n",d->m_tag,flag);
}

/* clear pending bit, if any ints are pending, then int will be triggered, otherwise it
will be cleared */
void ins8250_device_clear_int(struct ins8250_device *d, int flag)
{
	d->m_int_pending &= ~flag;
	ins8250_device_update_interrupt(d);
	LOG("8250 [%s] clear int source=%d\n",d->m_tag,flag);
}

// Baud rate generator is reset after writing to either byte of divisor latch
void ins8250_device_update_baud_rate(struct ins8250_device *d)
{
	//set_rate(clock(), d->m_regs.dl * 16);

	/* on a 16950, the SC divisor may be set to other values than 16, but this is purposefully 
	   not implemented since the ins8250_timer is 16X. Only the prescaler (CPR) is taken into account. */
	if (d->m_type >= TL16650 && (d->m_regs.mcr & 0x80))
	{
		double prescaler = (double)d->m_regs.icr[1]/8.0;
		d->m_brg_const = (uint16_t)(d->m_regs.dl *prescaler);
		LOG("8250 [%s] set baud rate DL=%04x, prescaler=%.3f, baud=%d\n",d->m_tag,d->m_regs.dl,prescaler,d->m_clock / (d->m_brg_const * 16));
	}
	else
	{
		d->m_brg_const = d->m_regs.dl;
		LOG("8250 [%s] set baud rate DL=%04x, baud=%d\n",d->m_tag,d->m_regs.dl,d->m_clock / (d->m_brg_const * 16));
	}
	d->tx_timer = d->m_brg_const;

	// FIXME: Baud rate generator should not affect transmitter or receiver, but device_serial_interface resets them regardless.
	// If the transmitter is still running at this time and we don't flush it, the shift register will never be emptied!
	if (!(d->m_regs.lsr & INS8250_LSR_TSRE))
		ins8250_device_tra_complete(d);
}

void ins8250_device_set_data_frame(struct ins8250_device *ch, int data_bit_count, enum parity_t parity, enum stop_bits_t stop_bits)
{
	//device().logerror("Start bits: %d; Data bits: %d; Parity: %s; Stop bits: %s\n", start_bit_count, data_bit_count, parity_tostring(parity), stop_bits_tostring(stop_bits));

	int stop_bit_count;

	switch (stop_bits)
	{
	case STOP_BITS_0:
	default:
		stop_bit_count = 0;
		break;

	case STOP_BITS_1:
		stop_bit_count = 1;
		break;

	case STOP_BITS_1_5:
		stop_bit_count = 2; // TODO: support 1.5 stop bits
		break;

	case STOP_BITS_2:
		stop_bit_count = 2;
		break;
	}

	//m_df_parity = parity;
	//m_df_start_bit_count = start_bit_count;

	ch->m_bit_count = 1 + data_bit_count + stop_bit_count + (parity != PARITY_NONE?1:0);

}

void ins8250_device_tra_load(struct ins8250_device *d);

void ins8250_device_w( struct ins8250_device *d, offs_t offset, uint8_t data )
{
	int tmp;

	LOG("8250 [%s] register %x write: %02x\n",d->m_tag,offset,data);
	switch (offset)
	{
		case 0:
			if (d->m_regs.lcr & INS8250_LCR_DLAB)
			{
				d->m_regs.dl = (d->m_regs.dl & 0xff00) | data;
				ins8250_device_update_baud_rate(d);
			}
			else
			{
				d->m_regs.thr = data;
				d->m_regs.lsr &= ~INS8250_LSR_THRE;
				if((d->m_type >= NS16550) && (d->m_regs.fcr & 1))
					ns16550_device_push_tx(d,data);
				ins8250_device_clear_int(d,COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY);
				if(d->m_regs.lsr & INS8250_LSR_TSRE)
					ins8250_device_tra_load(d);
			}
			break;
		case 1:
			if (d->m_regs.lcr & INS8250_LCR_DLAB)
			{
				d->m_regs.dl = (d->m_regs.dl & 0xff) | (data << 8);
				ins8250_device_update_baud_rate(d);
			}
			else
			{
				if (d->m_type == OX16950 && (d->m_regs.icr[0] & 0x80))
				{
					d->m_regs.asr = data & 3;
				}
				else
				{
					if ((d->m_regs.lsr & INS8250_LSR_THRE) && (data & COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY))
						ins8250_device_trigger_int(d,COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY);
					d->m_regs.ier = data;
					ins8250_device_update_interrupt(d);
				}
			}
			break;
		case 2:
			if (d->m_type>=TL16650 && d->m_650eregs)
			{
				d->m_regs.efr = data;
				LOG("8250 [%s] EFR write\n",d->m_tag);
			}
			else
			{
				ns16550_device_set_fcr(d,data);
			}
			break;
		case 3:
			if (d->m_type>=TL16650 && data==0xbf)
			{
				d->m_regs.lcr |= INS8250_LCR_DLAB;
				d->m_650eregs = 1;
				LOG("8250 [%s] enable 650 regs\n",d->m_tag);
			} 
			else
			{
				d->m_regs.lcr = data;
				d->m_650eregs = 0;
				LOG("8250 [%s] disable 650 regs\n",d->m_tag);

				int data_bit_count = (d->m_regs.lcr & INS8250_LCR_BITCOUNT_MASK) + 5;
				enum parity_t parity;
				enum stop_bits_t stop_bits;

				switch ((d->m_regs.lcr>>3) & 7)
				{
				case 1:
					parity = PARITY_ODD;
					break;

				case 3:
					parity = PARITY_EVEN;
					break;

				case 5:
					parity = PARITY_MARK;
					break;

				case 7:
					parity = PARITY_SPACE;
					break;

				default:
					parity = PARITY_NONE;
					break;
				}

				if (!(d->m_regs.lcr & INS8250_LCR_2STOP_BITS))
					stop_bits = STOP_BITS_1;
				else if (data_bit_count == 5)
					stop_bits = STOP_BITS_1_5;
				else
					stop_bits = STOP_BITS_2;

				ins8250_device_set_data_frame(d, data_bit_count, parity, stop_bits);
			}
			break;
		case 4:
			if ( d->m_type < TL16650 || (d->m_type >= TL16650 && !d->m_650eregs))
			{
				// write MCR
				if ( (d->m_type < TL16650 || !(d->m_regs.efr & 0x10)) && ( d->m_regs.mcr & 0x1f ) != ( data & 0x1f ) )
				{
					d->m_regs.mcr = data & 0x1f;
				}
				else if (d->m_regs.mcr != data)
				{
					d->m_regs.mcr = data;
				}
				else 
				{
					goto no_mcr_update;
				}

				ins8250_device_update_msr(d);

				if (d->m_regs.mcr & INS8250_MCR_LOOPBACK)
				{
					//d->m_out_tx_cb(d,1);
					//device_serial_interface::rx_w(d->m_txd);
					//d->m_out_dtr_cb(d,1);
					//d->m_out_rts_cb(d,1);
					//d->m_out_out1_cb(d,1);
					//d->m_out_out2_cb(d,1);
				}
				else
				{
					//d->m_out_tx_cb(d,d->m_txd);
					//device_serial_interface::rx_w(d->m_rxd);
					//d->m_out_dtr_cb(d,(d->m_regs.mcr & INS8250_MCR_DTR) ? 0 : 1);
					//d->m_out_rts_cb(d,(d->m_regs.mcr & INS8250_MCR_RTS) ? 0 : 1);
					//d->m_out_out1_cb(d,(d->m_regs.mcr & INS8250_MCR_OUT1) ? 0 : 1);
					//d->m_out_out2_cb(d,(d->m_regs.mcr & INS8250_MCR_OUT2) ? 0 : 1);
				}
			} 
			else
			{
				d->m_regs.xon1 = data;
			}
no_mcr_update:
			break;
		case 5:
			if (d->m_type>=TL16650)
			{
				if (d->m_650eregs)
				{
					d->m_regs.xon2 = data;
				}
				else
				{
					// write ICR
					if (d->m_regs.scr<8 ||(d->m_regs.scr>=0xc &&d->m_regs.scr<=0xe) ||d->m_regs.scr==0x11 ||d->m_regs.scr==0x13)
					{
						d->m_regs.icr[d->m_regs.scr] = data;
						LOG("8250 [%s] ICR %x write\n",d->m_tag,d->m_regs.scr);
					}

					if (d->m_regs.scr==0xc && data==0)
					{
						ins8250_device_reset(d);
					}
					else if (d->m_regs.scr==1)
					{
						ins8250_device_update_baud_rate(d);
					}
				}
				// no way to write to LSR on 650
			}
			else
			{
				/*
				  This register can be written, but if you write a 1 bit into any of
				  bits 5 - 0, you could cause an interrupt if the appropriate IER bit
				  is set.
				*/
				d->m_regs.lsr = (d->m_regs.lsr & (INS8250_LSR_TSRE|INS8250_LSR_THRE)) | (data & ~(INS8250_LSR_TSRE|INS8250_LSR_THRE));

				tmp = 0;
				tmp |= ( d->m_regs.lsr & INS8250_LSR_DR ) ? COM_INT_PENDING_RECEIVED_DATA_AVAILABLE : 0;
				tmp |= ( d->m_regs.lsr & 0x1e ) ? COM_INT_PENDING_RECEIVER_LINE_STATUS : 0;
				tmp |= ( d->m_regs.lsr & INS8250_LSR_THRE ) ? COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY : 0;
				ins8250_device_trigger_int(d,tmp);
			}
			break;
		case 6:
			if (d->m_type>=TL16650 && d->m_650eregs)
			{
				d->m_regs.xoff1 = data; 
				// no way to write to MSR on 650
			}
			else
			{
				/*
				  This register can be written, but if you write a 1 bit into any of
				  bits 3 - 0, you could cause an interrupt if the appropriate IER bit
				  is set.
				 */
				d->m_regs.msr = data;

				if ( d->m_regs.msr & 0x0f )
					ins8250_device_trigger_int(d,COM_INT_PENDING_MODEM_STATUS_REGISTER);
			}
			break;
		case 7:
			if (d->m_type>=TL16650 && d->m_650eregs)
			{
				d->m_regs.xoff2 = data;
			}
			else
			{
				d->m_regs.scr = data;
			}
			break;
	}
}

uint8_t ins8250_device_r(struct ins8250_device *d, offs_t offset)
{
	int data = 0x0ff;

	switch (offset)
	{
		case 0:
			if (d->m_regs.lcr & INS8250_LCR_DLAB)
				data = (d->m_regs.dl & 0xff);
			else
			{
				if((d->m_type >= NS16550) && (d->m_regs.fcr & 1))
					d->m_regs.rbr = ns16550_device_pop_rx(d);
				else
				{
					ins8250_device_clear_int(d,COM_INT_PENDING_RECEIVED_DATA_AVAILABLE);
					if( d->m_regs.lsr & INS8250_LSR_DR )
						d->m_regs.lsr &= ~INS8250_LSR_DR;
				}
				data = d->m_regs.rbr;
			}
			break;
		case 1:
			if (d->m_regs.lcr & INS8250_LCR_DLAB)
			{
				data = (d->m_regs.dl >> 8);
			}
			else
			{
				if (d->m_type == OX16950 && (d->m_regs.icr[0] & 0x80))
				{
					data = d->m_regs.asr;
				}
				else
				{
					data = d->m_regs.ier & 0x0f;
				}
			}
			break;
		case 2:
			if (d->m_type>=TL16650 && d->m_650eregs)
			{
				data = d->m_regs.efr;
				LOG("8250 [%s] EFR read\n",d->m_tag);
			}
			else
			{
				data = d->m_regs.iir;
				/* The documentation says that reading this register will
				clear the int if this is the source of the int */
				if ( (data & 0xf) == 0x02 )
					ins8250_device_clear_int(d,COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY);
			}
			break;
		case 3:
			if (d->m_type==OX16950 && (d->m_regs.icr[0] & 0x80))
			{
				data = d->m_rnum;
			}
			else
			{
				data = d->m_regs.lcr;
			}
			break;
		case 4:
			if (d->m_type>=TL16650 && d->m_650eregs)
			{
				data = d->m_regs.xon1;
			}
			else if (d->m_type==OX16950 && !d->m_650eregs)
			{
				data = d->m_thead - d->m_ttail;
			}
			else
			{
				data = d->m_regs.mcr;
			}
			break;
		case 5:
			if (d->m_type>=TL16650)
			{
				if (d->m_650eregs)
				{
					data = d->m_regs.xon2;
				}
				else if (d->m_regs.icr[0]&0x40)
				{
					if (d->m_regs.scr==0xf)
					{
						data = d->m_regs.fcr;
					}
					else
					{
						data = d->m_regs.icr[d->m_regs.scr];
					}
					LOG("8250 [%s] ICR %x read\n",d->m_tag,d->m_regs.scr);
				}
				else
					goto readlsr;
			}
			else
			{
readlsr:
				data = d->m_regs.lsr;
				if( d->m_regs.lsr & 0x1f )
					d->m_regs.lsr &= 0xe1; /* clear FE, PE and OE and BREAK bits */

				/* reading line status register clears int */
				ins8250_device_clear_int(d,COM_INT_PENDING_RECEIVER_LINE_STATUS);
			}
			break;
		case 6:
			if (d->m_type>=TL16650 && d->m_650eregs)
			{
				data = d->m_regs.xoff1;
			}
			else
			{
				data = d->m_regs.msr;
				d->m_regs.msr &= 0xf0; /* reset delta values */

				/* reading msr clears int */
				ins8250_device_clear_int(d,COM_INT_PENDING_MODEM_STATUS_REGISTER);
			}
			break;
		case 7:
			if (d->m_type>=TL16650 && d->m_650eregs)
			{
				data = d->m_regs.xoff2;
			}
			else
			{
				data = d->m_regs.scr;
			}
			break;
	}
	LOG("8250 [%s] register %x read: %02x\n",d->m_tag,offset,data);
	return data;
}

#define ins8250_device_fifo_and(d,fifo) { \
  if (d->m_type < TL16750) fifo &= 0x0f; \
  else if (d->m_type == TL16650) fifo &= 0x1f; \
  else if (d->m_type == TL16750) fifo &= 0x3f; \
  else fifo &= 0x7f; }
 

void _ins8250_device_rcv_complete(struct ins8250_device *d);

void ins8250_device_rcv_complete(struct ins8250_device *d) /* generic function for all device types */
{
	LOG("8250 [%s] rcv_complete\n",d->m_tag);
	if(d->m_type < NS16550 || !(d->m_regs.fcr & 1))
		return _ins8250_device_rcv_complete(d);

	receive_register_extract();

	if((d->m_type < TL16750 && d->m_rnum == 16) ||
	  (d->m_type == TL16650 && d->m_rnum == 32) ||
	  (d->m_type == TL16750 && d->m_rnum == 64) ||
	  (d->m_rnum == 128))
	{
		d->m_regs.lsr |= INS8250_LSR_OE; //overrun
		ins8250_device_trigger_int(d,COM_INT_PENDING_RECEIVER_LINE_STATUS);
		return;
	}

	d->m_regs.lsr |= INS8250_LSR_DR;
	d->m_rfifo[d->m_rhead] = get_received_char(d);
	++d->m_rhead;
	ins8250_device_fifo_and(d,d->m_rhead);
	d->m_rnum++;
	if(d->m_rnum >= d->m_rintlvl)
		ins8250_device_trigger_int(d,COM_INT_PENDING_RECEIVED_DATA_AVAILABLE);
	ns16550_device_set_timer(d);
}

void _ins8250_device_tra_load(struct ins8250_device *d);

void ins8250_device_tra_load(struct ins8250_device *d) /* generic function for all device types */
{
	if(d->m_type < NS16550 || !(d->m_regs.fcr & 1))
		return _ins8250_device_tra_load(d);

	if(d->m_ttail != d->m_thead)
	{
		transmit_register_setup(d,d->m_tfifo[d->m_ttail]);
		++d->m_ttail;
		ins8250_device_fifo_and(d,d->m_ttail);
		d->m_regs.lsr &= ~INS8250_LSR_TSRE;
		if(d->m_ttail == d->m_thead)
		{
			d->m_regs.lsr |= INS8250_LSR_THRE;
			ins8250_device_trigger_int(d,COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY);
		}
	}
	else
		d->m_regs.lsr |= INS8250_LSR_TSRE;
}

void ins8250_device_tra_complete(struct ins8250_device *d) /* generic function for all device types */
{
	LOG("8250 [%s] tra_complete\n",d->m_tag);
	if (d->tx_callback)
		d->tx_callback(d,0,d->tx_data);
	
	ins8250_device_tra_load(d);
}

void _ins8250_device_rcv_complete(struct ins8250_device *d) /* 8250/no fifo receive */
{
	if(d->m_regs.lsr & INS8250_LSR_DR)
	{
		d->m_regs.lsr |= INS8250_LSR_OE; //overrun
		ins8250_device_trigger_int(d,COM_INT_PENDING_RECEIVER_LINE_STATUS);
		receive_register_reset(d);
	}
	else
	{
		d->m_regs.lsr |= INS8250_LSR_DR;
		receive_register_extract(d);
		d->m_regs.rbr = get_received_char(d);
		ins8250_device_trigger_int(d,COM_INT_PENDING_RECEIVED_DATA_AVAILABLE);
	}
}

void _ins8250_device_tra_load(struct ins8250_device *d) /* 8250/no fifo transmit */
{
	if(!(d->m_regs.lsr & INS8250_LSR_THRE))
	{
		transmit_register_setup(d,d->m_regs.thr);
		d->m_regs.lsr &= ~INS8250_LSR_TSRE;
		d->m_regs.lsr |= INS8250_LSR_THRE;
		ins8250_device_trigger_int(d,COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY);
	}
	else
		d->m_regs.lsr |= INS8250_LSR_TSRE;
}

void ins8250_device_tra_callback(struct ins8250_device *d)
{
	LOG("8250 [%s] tra_callback b:%d\n",d->m_tag,d->tx_bits_rem);
	if (!is_transmit_register_empty(d)) {
		d->m_txd = transmit_register_get_data_bit(d);
		if (d->m_regs.mcr & INS8250_MCR_LOOPBACK)
		{
			//device_serial_interface::rx_w(d->m_txd);
		}
		else
		{
			//d->m_out_tx_cb(d,d->m_txd);
		}
		if (!--d->tx_bits_rem)
			ins8250_device_tra_complete(d);
	}
}

void ins8250_device_rcv_callback(struct ins8250_device *d)
{
	int c = -1;
	LOG("8250 [%s] rcv_callback b:%d\n",d->m_tag,d->rx_bits_rem);
	if (d->rx_bits_rem > 0) {
		if (!--d->rx_bits_rem) {
			ins8250_device_rcv_complete(d);
		}
	} else {
		if (d->rx_callback)
			c = d->rx_callback(d,0);
		if (c!=-1) {
			d->rx_data = c;
			d->rx_bits_rem = d->m_bit_count;
		}
	}
}

void ins8250_device_update_msr(struct ins8250_device *d)
{
	uint8_t data;
	int change;

	if (d->m_regs.mcr & INS8250_MCR_LOOPBACK)
	{
		data = (((d->m_regs.mcr & (INS8250_MCR_OUT1|INS8250_MCR_OUT2)) << 4) | \
			((d->m_regs.mcr & INS8250_MCR_DTR) << 5) | ((d->m_regs.mcr & INS8250_MCR_RTS) << 3));
		change = (d->m_regs.msr ^ data) >> 4;
		if(!(d->m_regs.msr & 0x40) && (data & 0x40))
			change &= ~4;
	}
	else
	{
		data = (!d->m_dcd << 7) | (!d->m_ri << 6) | (!d->m_dsr << 5) | (!d->m_cts << 4);
		change = (d->m_regs.msr ^ data) >> 4;
	}

	d->m_regs.msr = data | change;

	if(change)
		ins8250_device_trigger_int(d,COM_INT_PENDING_MODEM_STATUS_REGISTER);
}

/*WRITE_LINE_MEMBER(ins8250_uart_device::dcd_w)
{
	m_dcd = state;
	update_msr();
}

WRITE_LINE_MEMBER(ins8250_uart_device::dsr_w)
{
	m_dsr = state;
	update_msr();
}

WRITE_LINE_MEMBER(ins8250_uart_device::ri_w)
{
	m_ri = state;
	update_msr();
}

WRITE_LINE_MEMBER(ins8250_uart_device::cts_w)
{
	m_cts = state;
	update_msr();
}

WRITE_LINE_MEMBER(ins8250_uart_device::rx_w)
{
	m_rxd = state;

	if (!(m_regs.mcr & INS8250_MCR_LOOPBACK))
		device_serial_interface::rx_w(m_rxd);
}

void ins8250_device_start(struct ins8250_device *d)
{
	m_out_tx_cb.resolve_safe();
	m_out_dtr_cb.resolve_safe();
	m_out_rts_cb.resolve_safe();
	m_out_int_cb.resolve_safe();
	m_out_out1_cb.resolve_safe();
	m_out_out2_cb.resolve_safe();
	set_tra_rate(0);
	set_rcv_rate(0);

	save_item(NAME(m_regs.thr));
	save_item(NAME(m_regs.rbr));
	save_item(NAME(m_regs.ier));
	save_item(NAME(m_regs.dl));
	save_item(NAME(m_regs.iir));
	save_item(NAME(m_regs.fcr));
	save_item(NAME(m_regs.lcr));
	save_item(NAME(m_regs.mcr));
	save_item(NAME(m_regs.lsr));
	save_item(NAME(m_regs.msr));
	save_item(NAME(m_regs.scr));
	save_item(NAME(m_int_pending));
	save_item(NAME(m_txd));
	save_item(NAME(m_rxd));
	save_item(NAME(m_dcd));
	save_item(NAME(m_dsr));
	save_item(NAME(m_ri));
	save_item(NAME(m_cts));
}*/

void _ins8250_device_reset(struct ins8250_device *d)
{
	d->m_regs.ier = 0;
	d->m_regs.iir = 1;
	d->m_regs.lcr = 0;
	d->m_regs.mcr = 0;
	d->m_regs.lsr = INS8250_LSR_THRE | INS8250_LSR_TSRE;
	d->m_regs.dl = 1;
	ins8250_device_update_msr(d);
	d->m_regs.msr &= 0xf0;
	d->m_int_pending = 0;
	//ins8250_device_update_interrupt(d);
	receive_register_reset(d);
	transmit_register_reset(d);
	d->m_txd = 1;
	/*d->m_out_tx_cb(d,1);
	d->m_out_rts_cb(d,1);
	d->m_out_dtr_cb(d,1);
	d->m_out_out1_cb(d,1);
	d->m_out_out2_cb(d,1);*/
}

/*void ns16550_device::device_start()
{
	m_timeout = timer_alloc();
	ins8250_uart_device::device_start();
	save_item(NAME(m_rintlvl));
	save_item(NAME(m_rfifo));
	save_item(NAME(m_tfifo));
	save_item(NAME(m_rhead));
	save_item(NAME(m_rtail));
	save_item(NAME(m_rnum));
	save_item(NAME(m_thead));
	save_item(NAME(m_ttail));
}*/

void ins8250_device_reset(struct ins8250_device *d) /* generic function for all device types */
{
    LOG("8250 [%s] reset\n",d->m_tag);
	if (d->m_type >= NS16550) {
		memset(&d->m_rfifo, '\0', sizeof(d->m_rfifo));
		memset(&d->m_tfifo, '\0', sizeof(d->m_tfifo));
		d->m_rhead = d->m_rtail = d->m_rnum = 0;
		d->m_thead = d->m_ttail = 0;
		d->m_timeout = 0/*->adjust(attotime::never)*/;
	}
	_ins8250_device_reset(d);
	if (d->m_type == OX16950)
	{
		d->m_regs.asr = 0x80;
		d->m_regs.icr[0] = 0; /* ACR */
		d->m_regs.icr[1] = 0x20; /* CPR */
		d->m_regs.icr[2] = 0;
		d->m_regs.icr[3] = 0;
		d->m_regs.icr[4] = 0;
		d->m_regs.icr[5] = 0;
		d->m_regs.icr[6] = 0;
		d->m_regs.icr[7] = 0;

		/* ident */
		d->m_regs.icr[8] = 0x16;
		d->m_regs.icr[9] = 0xc9;
		d->m_regs.icr[10] = 0x54;
		d->m_regs.icr[11] = 0x04;

		d->m_regs.icr[13] = 0;
		d->m_regs.icr[14] = 0;
		d->m_regs.icr[15] = 0; /* RFC */
		d->m_regs.icr[16] = 1; /* GDS */
		d->m_regs.icr[17] = 2; /* DMS */
		d->m_regs.icr[19] = 0; /* CKA */
	}
	if (d->m_type == TL16750 || d->m_type == OX16950)
	{
		d->m_regs.mcr |= d->m_clksel?0:0x80;
	}
	if (d->m_type >= TL16750)
	{
		d->m_regs.fcr = 0;
	}
	if (d->m_type >= TL16650)
	{
		d->m_regs.efr = 0;
		d->m_regs.xon1 = 0;
		d->m_regs.xon2 = 0;
		d->m_regs.xoff1 = 0;
		d->m_regs.xoff2 = 0;
	}
	ins8250_device_update_baud_rate(d);
}

void pc16552_device_reset(struct pc16552_device *d)
{
	ins8250_device_reset(d->m_chan0);
	ins8250_device_reset(d->m_chan1);
}

void pc16554_device_reset(struct pc16554_device *d)
{
	ins8250_device_reset(d->m_chan0);
	ins8250_device_reset(d->m_chan1);
	ins8250_device_reset(d->m_chan2);
	ins8250_device_reset(d->m_chan3);
}

void ins8250_device_timer(struct ins8250_device *d /*, emu_timer *timer, device_timer_id id, int param, void *ptr*/)
{
	if (!--d->tx_timer)
	{ /* assume RCLK = BAUDOUT */
		if (d->m_type<OX16950)
		{
			// RX,TX is always enabled
			ins8250_device_rcv_callback(d);
			ins8250_device_tra_callback(d);
		}
		else
		{
			if (!(d->m_regs.icr[0]&1))
				ins8250_device_rcv_callback(d);
			if (!(d->m_regs.icr[0]&2))
				ins8250_device_tra_callback(d);
		}
		d->tx_timer = d->m_brg_const;
	}
             
	if(d->m_type >= NS16550 && !--d->m_timeout)
	{
		ins8250_device_trigger_int(d,COM_INT_PENDING_CHAR_TIMEOUT);
		d->m_timeout = 0/*->adjust(attotime::never)*/;
	}
}

void ns16550_device_push_tx(struct ins8250_device *d, uint8_t data)
{
	LOG("[%s] fifo push %02x\n",d->m_tag,data);
	d->m_tfifo[d->m_thead] = data;
	++d->m_thead;
	ins8250_device_fifo_and(d,d->m_thead);
}

uint8_t ns16550_device_pop_rx(struct ins8250_device *d)
{
	uint8_t data = d->m_rfifo[d->m_rtail];
	ins8250_device_clear_int(d,COM_INT_PENDING_CHAR_TIMEOUT & ~1); // don't clear bit 1 yet

	if(d->m_rnum)
	{
		++d->m_rtail;
		ins8250_device_fifo_and(d,d->m_rtail);
		d->m_rnum--;
	}
	else
		data = 0;

	if(d->m_rnum < d->m_rintlvl)
		ins8250_device_clear_int(d,COM_INT_PENDING_RECEIVED_DATA_AVAILABLE);

	if(d->m_rnum)
		ns16550_device_set_timer(d);
	else
	{
		d->m_timeout = 0/*->adjust(attotime::never)*/;
		d->m_regs.lsr &= ~INS8250_LSR_DR;
	}

	LOG("[%s] fifo pop %02x\n",d->m_tag,data);
	return data;
}

void ns16550_device_set_fcr(struct ins8250_device *d, uint8_t data)
{
	const int bytes_per_int[] = {1, 4, 8, 14};
	if(!(data & 1))
	{
		d->m_regs.fcr = 0;
		d->m_regs.iir &= ~0xc8;
		return;
	}
	if(!(d->m_regs.fcr & 1) && (data & 1))
		data |= 0x06;
	if(data & 2)
	{
		memset(&d->m_rfifo, '\0', sizeof(d->m_rfifo));
		d->m_rhead = d->m_rtail = d->m_rnum = 0;
		ins8250_device_clear_int(d,COM_INT_PENDING_CHAR_TIMEOUT | COM_INT_PENDING_RECEIVED_DATA_AVAILABLE);
		d->m_timeout = 0/*->adjust(attotime::never)*/;
	}
	if(data & 4)
	{
		memset(&d->m_tfifo, '\0', sizeof(d->m_tfifo));
		d->m_thead = d->m_ttail = 0;
		d->m_regs.lsr |= INS8250_LSR_THRE;
		ins8250_device_trigger_int(d,COM_INT_PENDING_TRANSMITTER_HOLDING_REGISTER_EMPTY);
	}
	d->m_rintlvl = bytes_per_int[(data>>6)&3];
	d->m_regs.iir |= 0xc0;
	d->m_regs.fcr = data & 0xc9;
}

void ns16550_device_set_timer(struct ins8250_device *d)
{
	// no characters have been popped from receiver fifo for 4 character cycle times
	//m_timeout->adjust(attotime::from_hz((clock()*4*8)/(m_regs.dl*16))); 
	d->m_timeout = 4*d->m_bit_count*d->m_clock/(d->m_brg_const*16);
}

uint8_t pc16552_device_r(struct pc16552_device *d, offs_t offset)
{
	return ins8250_device_r((offset & 8) ? d->m_chan1 : d->m_chan0, offset & 7);
}

void pc16552_device_w( struct pc16552_device *d, offs_t offset, uint8_t data )
{
	ins8250_device_w((offset & 8) ? d->m_chan1 : d->m_chan0, offset & 7, data);
}

uint8_t pc16554_device_r(struct pc16554_device *d, offs_t offset)
{
	switch(offset & 24)
	{
		case 0<<3:
			return ins8250_device_r(d->m_chan0, offset & 7);
		case 1<<3:
			return ins8250_device_r(d->m_chan1, offset & 7);
		case 2<<3:
			return ins8250_device_r(d->m_chan2, offset & 7);
		case 3<<3:
			return ins8250_device_r(d->m_chan3, offset & 7);
	}
}

void pc16554_device_w( struct pc16554_device *d, offs_t offset, uint8_t data )
{
	switch(offset & 24)
	{
		case 0<<3:
			return ins8250_device_w(d->m_chan0, offset & 7, data);
		case 1<<3:
			return ins8250_device_w(d->m_chan1, offset & 7, data);
		case 2<<3:
			return ins8250_device_w(d->m_chan2, offset & 7, data);
		case 3<<3:
			return ins8250_device_w(d->m_chan3, offset & 7, data);
	}
}
