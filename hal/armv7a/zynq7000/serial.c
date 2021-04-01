/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Zynq-7000 Serial driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczyński
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "peripherals.h"
#include "zynq.h"
#include "../serial.h"
#include "../errors.h"
#include "../timer.h"
#include "../hal.h"


#define MAX_TXRX_FIFO_SIZE  0x40
#define BUFFER_SIZE         0x200


typedef struct {
	volatile u32 *base;
	u16 irq;
	u16 clk;

	u16 rxFifoSz;
	u16 txFifoSz;

	u8 rxBuff[BUFFER_SIZE];
	u16 rxHead;
	u16 rxTail;

	u8 txBuff[BUFFER_SIZE];
	u16 txHead;
	u16 txTail;

	u8 tFull;
} serial_t;


enum {
	cr = 0, mr, ier, idr, imr, isr, baudgen, rxtout, rxwm, modemcr, modemsr, sr, fifo,
	baud_rate_divider_reg0, flow_delay_reg0, tx_fifo_trigger_level0,
};


struct {
	serial_t serials[UARTS_MAX_CNT];
} serial_common;


static inline void serial_rxData(serial_t *serial)
{
	/* Keep getting data until fifo is not empty */
	while (!(*(serial->base + sr) & (0x1 << 1))) {
		serial->rxBuff[serial->rxHead] = *(serial->base + fifo);
		serial->rxHead = (serial->rxHead + 1) % BUFFER_SIZE;

		if (serial->rxHead == serial->rxTail)
			serial->rxTail = (serial->rxTail + 1) % BUFFER_SIZE;
	}
}


static inline void serial_txData(serial_t *serial)
{
	/* Keep filling until fifo is not full */
	while (!(*(serial->base + sr) & (0x1 << 4))) {
		serial->txHead = (serial->txHead + 1) % BUFFER_SIZE;
		if (serial->txHead != serial->txTail) {
			*(serial->base + fifo) = serial->txBuff[serial->txHead];
			serial->tFull = 0;
		}
		else {
			/* Transfer is finished, disable irq */
			*(serial->base + idr) |= 1 << 3;
			break;
		}
	}
}


static int serial_irqHandler(u16 n, void *data)
{
	u32 st;
	serial_t *serial = (serial_t *)data;

	if (serial == NULL)
		return 0;

	st = *(serial->base + isr) & *(serial->base + imr);

	/* RX FIFO trigger irq */
	if (st & 0x1)
		serial_rxData(serial);

	/* TX FIFO empty irq   */
	if (st & (1 << 3))
		serial_txData(serial);

	/* Clear irq status    */
	*(serial->base + isr) = st;

	return 0;
}


int serial_read(unsigned int pn, u8 *buff, u16 len, u16 timeout)
{
	u16 l, cnt = 0;
	serial_t *serial;

	if (pn >= UARTS_MAX_CNT)
		return ERR_ARG;

	serial = &serial_common.serials[pn];

	if (!timer_wait(timeout, TIMER_VALCHG, &serial->rxHead, serial->rxTail))
		return ERR_SERIAL_TIMEOUT;

	hal_cli();

	if (serial->rxHead > serial->rxTail)
		l = min(serial->rxHead - serial->rxTail, len);
	else
		l = min(BUFFER_SIZE - serial->rxTail, len);

	hal_memcpy(buff, &serial->rxBuff[serial->rxTail], l);
	cnt = l;
	if ((len > l) && (serial->rxHead < serial->rxTail)) {
		hal_memcpy(buff + l, &serial->rxBuff[0], min(len - l, serial->rxHead));
		cnt += min(len - l, serial->rxHead);
	}
	serial->rxTail = ((serial->rxTail + cnt) % BUFFER_SIZE);

	hal_sti();

	return cnt;
}


int serial_write(unsigned int pn, const u8 *buff, u16 len)
{
	int l, cnt = 0;
	serial_t *serial;

	if (pn >= UARTS_MAX_CNT)
		return ERR_ARG;

	serial = &serial_common.serials[pn];

	while (serial->txHead == serial->txTail && serial->tFull)
		;

	hal_cli();
	if (serial->txHead > serial->txTail)
		l = min(serial->txHead - serial->txTail, len);
	else
		l = min(BUFFER_SIZE - serial->txTail, len);

	hal_memcpy(&serial->txBuff[serial->txTail], buff, l);
	cnt = l;
	if ((len > l) && (serial->txTail >= serial->txHead)) {
		hal_memcpy(serial->txBuff, buff + l, min(len - l, serial->txHead));
		cnt += min(len - l, serial->txHead);
	}

	/* Initialize sending */
	if (serial->txTail == serial->txHead)
		*(serial->base + fifo) = serial->txBuff[serial->txHead];

	serial->txTail = ((serial->txTail + cnt) % BUFFER_SIZE);

	if (serial->txTail == serial->txHead)
		serial->tFull = 1;

	/* Enable TX FIFO empty irq */
	 *(serial->base + ier) |= 1 << 3;
	hal_sti();

	return cnt;
}


int serial_safewrite(unsigned int pn, const u8 *buff, u16 len)
{
	int l;

	for (l = 0; len;) {
		if ((l = serial_write(pn, buff, len)) < 0)
			return ERR_MSG_IO;
		buff += l;
		len -= l;
	}

	return 0;
}


int serial_rxEmpty(unsigned int pn)
{
	serial_t *serial;

	if (pn >= UARTS_MAX_CNT)
		return ERR_ARG;

	serial = &serial_common.serials[pn];

	return serial->rxHead == serial->rxTail;
}


/* According to TRM:
*  baud_rate = ref_clk / (bgen * (bdiv + 1))
*  bgen: 2 - 65535
*  bdiv: 4 - 255                             */
static int serial_calcBaudarate(int pn, int baudrate)
{
	u32 bestDiff, diff;
	u32 calcBaudrate;
	u32 bdiv, bgen, bestBdiv = 4, bestBgen = 2;

	serial_t *serial;

	if (pn >= UARTS_MAX_CNT)
		return ERR_ARG;

	serial = &serial_common.serials[pn];

	bestDiff = (u32)baudrate;

	for (bdiv = 4; bdiv < 255; bdiv++) {
		bgen = UART_REF_CLK / (baudrate * (bdiv + 1));

		if (bgen < 2 || bgen > 65535)
			continue;

		calcBaudrate = UART_REF_CLK / (bgen * (bdiv + 1));

		if (calcBaudrate > baudrate)
			diff = calcBaudrate - baudrate;
		else
			diff = baudrate - calcBaudrate;

		if (diff < bestDiff) {
			bestDiff = diff;
			bestBdiv = bdiv;
			bestBgen = bgen;
		}
	}

	*(serial->base + baudgen) = bestBgen;
	*(serial->base + baud_rate_divider_reg0) = bestBdiv;

	return ERR_NONE;
}


static int serial_setPin(u32 pin)
{
	ctl_mio_t ctl;

	/* Set default properties for UART's pins */
	ctl.pin = pin;
	ctl.l0 = ctl.l1 = ctl.l2 = 0;
	ctl.l3 = 0x7;
	ctl.speed = 0;
	ctl.ioType = 1;
	ctl.pullup = 0;
	ctl.disableRcvr = 0;

	switch (pin)
	{
		/* Uart Rx */
		case mio_pin_10:
		case mio_pin_49:
			ctl.triEnable = 1;
			break;

		/* Uart Tx */
		case mio_pin_11:
		case mio_pin_48:
			ctl.triEnable = 0;
			break;

		default:
			return ERR_ARG;
	}

	return _zynq_setMIO(&ctl);
}


static void serial_initCtrlClock(void)
{
	ctl_clock_t ctl;

	/* Set IO PLL as source clock and set divider:
	 * IO_PLL / 0x14 :  1000 MHz / 20 = 50 MHz     */
	ctl.dev = ctrl_uart_clk;
	ctl.pll.clkact0 = 0x1;
	ctl.pll.clkact1 = 0x1;
	ctl.pll.srcsel = 0;
	ctl.pll.divisor0 = 0x14;

	_zynq_setCtlClock(&ctl);
}


u32 serial_getBaudrate(void)
{
	return UART_BAUDRATE;
}


void serial_init(void)
{
	int i;
	serial_t *serial;

	static const struct {
		volatile u32 *base;
		u16 irq;
		u16 clk;
		u16 rxPin;
		u16 txPin;
	} info[UARTS_MAX_CNT] = {
		{ UART0_BASE_ADDR, UART0_IRQ, UART0_CLK, UART0_RX, UART0_TX },
		{ UART1_BASE_ADDR, UART1_IRQ, UART1_CLK, UART1_RX, UART1_TX }
	};


	/* UART Clock Controller configuration */
	serial_initCtrlClock();

	for (i = 0; i < UARTS_MAX_CNT; ++i) {
		if (_zynq_setAmbaClk(info[i].clk, clk_enable) < 0)
			return;

		if (serial_setPin(info[i].rxPin) < 0 || serial_setPin(info[i].txPin) < 0)
			return;

		serial = &serial_common.serials[i];

		serial->rxHead = 0;
		serial->txHead = 0;
		serial->rxTail = 0;
		serial->txTail = 0;
		serial->tFull = 0;

		serial->irq = info[i].irq;
		serial->clk = info[i].clk;
		serial->base = info[i].base;

		/* Reset RX & TX */
		*(serial->base + cr) = 0x3;

		/* Uart Mode Register
		 * normal mode, 1 stop bit, no parity, 8 bits, uart_ref_clk as source clock
		 * PAR = 0x4 */
		*(serial->base + mr) = (*(serial->base + mr) & ~0x000003ff) | 0x00000020;

		/* Disable TX and RX */
		*(serial->base + cr) = (*(serial->base + cr) & ~0x000001ff) | 0x00000028;

		serial_calcBaudarate(i, UART_BAUDRATE);

		/* Uart Control Register
		 * TXEN = 0x1; RXEN = 0x1; TXRES = 0x1; RXRES = 0x1 */
		*(serial->base + cr) = (*(serial->base + cr) & ~0x000001ff) | 0x00000017;

		hal_irqinst(info[i].irq, serial_irqHandler, (void *)serial);

		/* Enable RX FIFO trigger */
		*(serial->base + ier) |= 0x1;
		/* Set trigger level, range: 1-63 */
		*(serial->base + rxwm) = 1;
	}
}


void serial_done(void)
{
	int i;
	serial_t *serial;

	for (i = 0; i < UARTS_MAX_CNT; ++i) {
		serial = &serial_common.serials[i];
		/* Disable interrupts */
		*(serial->base + idr) = 0xfff;

		/* Disable RX & TX */
		*(serial->base + cr) = (1 << 5) | (1 << 3);
		/* Reset RX & TX */
		*(serial->base + cr) = 0x3;

		/* Clear status flags */
		*(serial->base + isr) = 0xfff;

		hal_irquninst(serial->irq);
		_zynq_setAmbaClk(serial->clk, clk_disable);
	}
}
