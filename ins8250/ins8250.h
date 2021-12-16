// license:BSD-3-Clause
// copyright-holders:smf, Carl
/**********************************************************************

    8250 UART interface and emulation

**********************************************************************/

#ifndef _INS8250_H
#define _INS8250_H

#pragma once

int VERBOSE;
#include <stdio.h>
#ifndef LOG
#define LOG(...) do { if (VERBOSE) printf (__VA_ARGS__); } while (0)
#endif

//#include "diserial.h"
#include <stdint.h>
typedef void device_t;
typedef uint32_t offs_t;
typedef uint16_t emu_timer;

/***************************************************************************
    CLASS DEFINITIONS
***************************************************************************/

/*class ins8250_uart_device : public device_t, public device_serial_interface
{
public:
	template <class Object> devcb_base &set_out_tx_callback(Object &&cb) { return m_out_tx_cb.set_callback(std::forward<Object>(cb)); }
	template <class Object> devcb_base &set_out_dtr_callback(Object &&cb) { return m_out_dtr_cb.set_callback(std::forward<Object>(cb)); }
	template <class Object> devcb_base &set_out_rts_callback(Object &&cb) { return m_out_rts_cb.set_callback(std::forward<Object>(cb)); }
	template <class Object> devcb_base &set_out_int_callback(Object &&cb) { return m_out_int_cb.set_callback(std::forward<Object>(cb)); }
	template <class Object> devcb_base &set_out_out1_callback(Object &&cb) { return m_out_out1_cb.set_callback(std::forward<Object>(cb)); }
	template <class Object> devcb_base &set_out_out2_callback(Object &&cb) { return m_out_out2_cb.set_callback(std::forward<Object>(cb)); }
	auto out_tx_callback() { return m_out_tx_cb.bind(); }
	auto out_dtr_callback() { return m_out_dtr_cb.bind(); }
	auto out_rts_callback() { return m_out_rts_cb.bind(); }
	auto out_int_callback() { return m_out_int_cb.bind(); }
	auto out_out1_callback() { return m_out_out1_cb.bind(); }
	auto out_out2_callback() { return m_out_out2_cb.bind(); }

	DECLARE_WRITE_LINE_MEMBER( dcd_w );
	DECLARE_WRITE_LINE_MEMBER( dsr_w );
	DECLARE_WRITE_LINE_MEMBER( ri_w );
	DECLARE_WRITE_LINE_MEMBER( cts_w );
	DECLARE_WRITE_LINE_MEMBER( rx_w );

protected:
*/
	enum ins8250_dev_type {
		INS8250 = (uint32_t)0,
		INS8250A,
		NS16450,
		NS16550,
		NS16550A,
		TL16750,
		TL16650,
		OX16950
	};

#ifndef __Z80COMMON_H__
#undef PARITY_NONE
#undef PARITY_ODD
#undef PARITY_EVEN
#undef PARITY_MARK
#undef PARITY_SPACE

	enum parity_t
	{
		PARITY_NONE,     /* no parity. a parity bit will not be in the transmitted/received data */
		PARITY_ODD,      /* odd parity */
		PARITY_EVEN,     /* even parity */
		PARITY_MARK,     /* one parity */
		PARITY_SPACE     /* zero parity */
	};

	enum stop_bits_t
	{
		STOP_BITS_0,
		STOP_BITS_1 = 1,
		STOP_BITS_1_5 = 2,
		STOP_BITS_2 = 3
	};
#endif

typedef void (*tx_callback_t)(device_t *device, int channel, uint8_t data);
typedef int (*rx_callback_t)(device_t *device, int channel);
/*ATTR_DEPRECATED*/ typedef void (*devcb_write_line)(device_t *device, int state);

/*	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void rcv_complete() override;
	virtual void tra_complete() override;
	virtual void tra_callback() override;

	virtual void set_fcr(uint8_t data) { }
	virtual void push_tx(uint8_t data) { }
	virtual uint8_t pop_rx() { return 0; }

	void trigger_int(int flag);
	void clear_int(int flag);

	void ins8250_update_baud_rate();
*/

	struct ins8250_device
	{
	char *m_tag;
	uint32_t m_type;
	uint32_t m_clock;
	void *m_owner; 

	struct {
		uint8_t thr;  /* 0 -W transmitter holding register */
		uint8_t rbr;  /* 0 R- receiver buffer register */
		uint8_t ier;  /* 1 RW interrupt enable register */
		uint16_t dl;  /* 0/1 RW divisor latch (if DLAB = 1) */
		uint8_t iir;  /* 2 R- interrupt identification register */
		uint8_t fcr;
		uint8_t lcr;  /* 3 RW line control register (bit 7: DLAB) */
		uint8_t mcr;  /* 4 RW modem control register */
		uint8_t lsr;  /* 5 R- line status register */
		uint8_t msr;  /* 6 R- modem status register */
		uint8_t scr;  /* 7 RW scratch register */
		/* 16650 */
		uint8_t efr;
		uint8_t xon1;
		uint8_t xon2;
		uint8_t xoff1;
		uint8_t xoff2;
		/* 16950 */
		uint8_t asr;
		uint8_t icr[0x14];
	} m_regs;
//private:
	uint8_t m_650eregs;	/* lcr 0xbf latch */
	uint8_t m_int_pending;

	/*devcb_write_line    m_out_tx_cb;
	devcb_write_line    m_out_dtr_cb;
	devcb_write_line    m_out_rts_cb;
	devcb_write_line    m_out_int_cb;
	devcb_write_line    m_out_out1_cb;
	devcb_write_line    m_out_out2_cb;*/

	/*void update_interrupt();
	void update_msr();*/

	int m_txd;
	int m_rxd;
	int m_dcd;
	int m_dsr;
	int m_ri;
	int m_cts;
    
	emu_timer tx_timer;
	emu_timer m_brg_const;

	uint8_t rx_bits_rem;
	uint8_t rx_data;

	uint8_t tx_bits_rem;
	uint8_t tx_data;

	uint8_t m_bit_count;
 
	// byte rx/tx callbacks
	// Note: bit rx/tx callbacks are not implemented
	tx_callback_t tx_callback;
	rx_callback_t rx_callback;
    
	// interrupt line callback
	devcb_write_line    m_out_int_cb;

	int m_clksel;
//};

/*class ins8250_device : public ins8250_uart_device
{
public:
	ins8250_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
};

class ns16450_device : public ins8250_uart_device
{
public:
	ns16450_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
};

class ns16550_device : public ins8250_uart_device
{
public:
	ns16550_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
protected:
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void rcv_complete() override;
	virtual void tra_complete() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;
	virtual void set_fcr(uint8_t data) override;
	virtual void push_tx(uint8_t data) override;
	virtual uint8_t pop_rx() override;
private:
*/
	int m_rintlvl;
	uint8_t m_rfifo[128];
	uint8_t m_tfifo[128];
	int m_rhead, m_rtail, m_rnum;
	int m_thead, m_ttail;
	uint64_t m_timeout;
};

/*class pc16552_device : public device_t
{
public:
	pc16552_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	virtual void device_start() override;

private:
*/
struct pc16552_device {
	struct ins8250_device *m_chan0;
	struct ins8250_device *m_chan1;
};

struct pc16554_device {
	struct ins8250_device *m_chan0;
	struct ins8250_device *m_chan1;
	struct ins8250_device *m_chan2;
	struct ins8250_device *m_chan3;
};

struct ins8250_device* ins8250_device_create(char *tag, device_t *owner, uint32_t clock, enum ins8250_dev_type device_type,
	devcb_write_line out_int_cb,
	rx_callback_t rx_callback,tx_callback_t tx_callback, int clksel);
struct pc16552_device* pc16552_device_create(char *tag, device_t *owner, uint32_t clock, enum ins8250_dev_type device_type,
	devcb_write_line out_int_cb,
	rx_callback_t rx_callback,tx_callback_t tx_callback, int clksel);
struct pc16554_device* pc16554_device_create(char *tag, device_t *owner, uint32_t clock, enum ins8250_dev_type device_type,
	devcb_write_line out_int_cb,
	rx_callback_t rx_callback,tx_callback_t tx_callback, int clksel);

uint8_t  ins8250_device_r(struct ins8250_device *d, offs_t offset);
void   ins8250_device_w(struct ins8250_device *d, offs_t offset, uint8_t data );

uint8_t  pc16552_device_r(struct pc16552_device *d, offs_t offset);
void   pc16552_device_w(struct pc16552_device *d, offs_t offset, uint8_t data );

uint8_t  pc16554_device_r(struct pc16554_device *d, offs_t offset);
void   pc16554_device_w(struct pc16554_device *d, offs_t offset, uint8_t data );

void ins8250_device_reset(struct ins8250_device *d);
void pc16552_device_reset(struct pc16552_device *d);
void pc16554_device_reset(struct pc16554_device *d);

void ins8250_device_timer(struct ins8250_device *d);

/*DECLARE_DEVICE_TYPE(PC16552D, pc16552_device)
DECLARE_DEVICE_TYPE(INS8250,  ins8250_device)
DECLARE_DEVICE_TYPE(NS16450,  ns16450_device)
DECLARE_DEVICE_TYPE(NS16550,  ns16550_device)*/

#endif // _INS8250_H
