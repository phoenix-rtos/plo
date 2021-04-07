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

#include "../../errors.h"
#include "../../timer.h"
#include "../../hal.h"

#include "peripherals.h"
#include "zynq.h"
#include "uart.h"


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
} uart_t;


enum {
	cr = 0, mr, ier, idr, imr, isr, baudgen, rxtout, rxwm, modemcr, modemsr, sr, fifo,
	baud_rate_divider_reg0, flow_delay_reg0, tx_fifo_trigger_level0,
};


struct {
	uart_t uarts[UARTS_MAX_CNT];
} uart_common;


static inline void uart_rxData(uart_t *uart)
{
	/* Keep getting data until fifo is not empty */
	while (!(*(uart->base + sr) & (0x1 << 1))) {
		uart->rxBuff[uart->rxHead] = *(uart->base + fifo);
		uart->rxHead = (uart->rxHead + 1) % BUFFER_SIZE;

		if (uart->rxHead == uart->rxTail)
			uart->rxTail = (uart->rxTail + 1) % BUFFER_SIZE;
	}
}


static inline void uart_txData(uart_t *uart)
{
	/* Keep filling until fifo is not full */
	while (!(*(uart->base + sr) & (0x1 << 4))) {
		uart->txHead = (uart->txHead + 1) % BUFFER_SIZE;
		if (uart->txHead != uart->txTail) {
			*(uart->base + fifo) = uart->txBuff[uart->txHead];
			uart->tFull = 0;
		}
		else {
			/* Transfer is finished, disable irq */
			*(uart->base + idr) |= 1 << 3;
			break;
		}
	}
}


static int uart_irqHandler(u16 n, void *data)
{
	u32 st;
	uart_t *uart = (uart_t *)data;

	if (uart == NULL)
		return 0;

	st = *(uart->base + isr) & *(uart->base + imr);

	/* RX FIFO trigger irq */
	if (st & 0x1)
		uart_rxData(uart);

	/* TX FIFO empty irq   */
	if (st & (1 << 3))
		uart_txData(uart);

	/* Clear irq status    */
	*(uart->base + isr) = st;

	return 0;
}


int uart_read(unsigned int pn, u8 *buff, u16 len, u16 timeout)
{
	u16 l, cnt = 0;
	uart_t *uart;

	if (pn >= UARTS_MAX_CNT)
		return ERR_ARG;

	uart = &uart_common.uarts[pn];

	if (!timer_wait(timeout, TIMER_VALCHG, &uart->rxHead, uart->rxTail))
		return ERR_UART_TIMEOUT;

	hal_cli();

	if (uart->rxHead > uart->rxTail)
		l = min(uart->rxHead - uart->rxTail, len);
	else
		l = min(BUFFER_SIZE - uart->rxTail, len);

	hal_memcpy(buff, &uart->rxBuff[uart->rxTail], l);
	cnt = l;
	if ((len > l) && (uart->rxHead < uart->rxTail)) {
		hal_memcpy(buff + l, &uart->rxBuff[0], min(len - l, uart->rxHead));
		cnt += min(len - l, uart->rxHead);
	}
	uart->rxTail = ((uart->rxTail + cnt) % BUFFER_SIZE);

	hal_sti();

	return cnt;
}


int uart_write(unsigned int pn, const u8 *buff, u16 len)
{
	int l, cnt = 0;
	uart_t *uart;

	if (pn >= UARTS_MAX_CNT)
		return ERR_ARG;

	uart = &uart_common.uarts[pn];

	while (uart->txHead == uart->txTail && uart->tFull)
		;

	hal_cli();
	if (uart->txHead > uart->txTail)
		l = min(uart->txHead - uart->txTail, len);
	else
		l = min(BUFFER_SIZE - uart->txTail, len);

	hal_memcpy(&uart->txBuff[uart->txTail], buff, l);
	cnt = l;
	if ((len > l) && (uart->txTail >= uart->txHead)) {
		hal_memcpy(uart->txBuff, buff + l, min(len - l, uart->txHead));
		cnt += min(len - l, uart->txHead);
	}

	/* Initialize sending */
	if (uart->txTail == uart->txHead)
		*(uart->base + fifo) = uart->txBuff[uart->txHead];

	uart->txTail = ((uart->txTail + cnt) % BUFFER_SIZE);

	if (uart->txTail == uart->txHead)
		uart->tFull = 1;

	/* Enable TX FIFO empty irq */
	 *(uart->base + ier) |= 1 << 3;
	hal_sti();

	return cnt;
}


int uart_safewrite(unsigned int pn, const u8 *buff, u16 len)
{
	int l;

	for (l = 0; len;) {
		if ((l = uart_write(pn, buff, len)) < 0)
			return ERR_MSG_IO;
		buff += l;
		len -= l;
	}

	return 0;
}


int uart_rxEmpty(unsigned int pn)
{
	uart_t *uart;

	if (pn >= UARTS_MAX_CNT)
		return ERR_ARG;

	uart = &uart_common.uarts[pn];

	return uart->rxHead == uart->rxTail;
}


/* According to TRM:
*  baud_rate = ref_clk / (bgen * (bdiv + 1))
*  bgen: 2 - 65535
*  bdiv: 4 - 255                             */
static int uart_calcBaudarate(int pn, int baudrate)
{
	u32 bestDiff, diff;
	u32 calcBaudrate;
	u32 bdiv, bgen, bestBdiv = 4, bestBgen = 2;

	uart_t *uart;

	if (pn >= UARTS_MAX_CNT)
		return ERR_ARG;

	uart = &uart_common.uarts[pn];

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

	*(uart->base + baudgen) = bestBgen;
	*(uart->base + baud_rate_divider_reg0) = bestBdiv;

	return ERR_NONE;
}


static int uart_setPin(u32 pin)
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


static void uart_initCtrlClock(void)
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


u32 uart_getBaudrate(void)
{
	return UART_BAUDRATE;
}


void uart_init(void)
{
	int i;
	uart_t *uart;

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
	uart_initCtrlClock();

	for (i = 0; i < UARTS_MAX_CNT; ++i) {
		if (_zynq_setAmbaClk(info[i].clk, clk_enable) < 0)
			return;

		if (uart_setPin(info[i].rxPin) < 0 || uart_setPin(info[i].txPin) < 0)
			return;

		uart = &uart_common.uarts[i];

		uart->rxHead = 0;
		uart->txHead = 0;
		uart->rxTail = 0;
		uart->txTail = 0;
		uart->tFull = 0;

		uart->irq = info[i].irq;
		uart->clk = info[i].clk;
		uart->base = info[i].base;

		/* Reset RX & TX */
		*(uart->base + cr) = 0x3;

		/* Uart Mode Register
		 * normal mode, 1 stop bit, no parity, 8 bits, uart_ref_clk as source clock
		 * PAR = 0x4 */
		*(uart->base + mr) = (*(uart->base + mr) & ~0x000003ff) | 0x00000020;

		/* Disable TX and RX */
		*(uart->base + cr) = (*(uart->base + cr) & ~0x000001ff) | 0x00000028;

		uart_calcBaudarate(i, UART_BAUDRATE);

		/* Uart Control Register
		 * TXEN = 0x1; RXEN = 0x1; TXRES = 0x1; RXRES = 0x1 */
		*(uart->base + cr) = (*(uart->base + cr) & ~0x000001ff) | 0x00000017;

		hal_irqinst(info[i].irq, uart_irqHandler, (void *)uart);

		/* Enable RX FIFO trigger */
		*(uart->base + ier) |= 0x1;
		/* Set trigger level, range: 1-63 */
		*(uart->base + rxwm) = 1;
	}
}


void uart_done(void)
{
	int i;
	uart_t *uart;

	for (i = 0; i < UARTS_MAX_CNT; ++i) {
		uart = &uart_common.uarts[i];
		/* Disable interrupts */
		*(uart->base + idr) = 0xfff;

		/* Disable RX & TX */
		*(uart->base + cr) = (1 << 5) | (1 << 3);
		/* Reset RX & TX */
		*(uart->base + cr) = 0x3;

		/* Clear status flags */
		*(uart->base + isr) = 0xfff;

		hal_irquninst(uart->irq);
		_zynq_setAmbaClk(uart->clk, clk_disable);
	}
}
