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

#include <hal/hal.h>
#include <hal/timer.h>
#include <lib/errno.h>
#include <lib/lib.h>
#include <devices/devs.h>


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
	int clkInit;
} uart_common;


/* TODO: specific uart information should be get from a device tree,
 *       using hardcoded defines is a temporary solution           */
const struct {
	volatile u32 *base;
	u16 irq;
	u16 clk;
	u16 rxPin;
	u16 txPin;
} info[UARTS_MAX_CNT] = {
	{ UART0_BASE_ADDR, UART0_IRQ, UART0_CLK, UART0_RX, UART0_TX },
	{ UART1_BASE_ADDR, UART1_IRQ, UART1_CLK, UART1_RX, UART1_TX }
};



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


/* According to TRM:
*  baud_rate = ref_clk / (bgen * (bdiv + 1))
*  bgen: 2 - 65535
*  bdiv: 4 - 255                             */
static int uart_calcBaudarate(uart_t *uart, int baudrate)
{
	u32 bestDiff, diff;
	u32 calcBaudrate;
	u32 bdiv, bgen, bestBdiv = 4, bestBgen = 2;

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

	return EOK;
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
			return -EINVAL;
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


/* Device interafce */

static ssize_t uart_read(unsigned int minor, addr_t offs, u8 *buff, unsigned int len, unsigned int timeout)
{
	u16 l, cnt = 0;
	uart_t *uart;

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	if (!timer_wait(timeout, TIMER_VALCHG, &uart->rxHead, uart->rxTail))
		return -ETIME;

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


static ssize_t uart_write(unsigned int minor, const u8 *buff, unsigned int len)
{
	int l, cnt = 0;
	uart_t *uart;

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

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


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const u8 *buff, unsigned int len)
{
	unsigned int l;

	for (l = 0; len;) {
		if ((l = uart_write(minor, buff, len)) < 0)
			return -ENXIO;
		buff += l;
		len -= l;
	}

	return len;
}


static int uart_sync(unsigned int minor)
{
	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	/* TBD */

	return EOK;
}


static int uart_done(unsigned int minor)
{
	uart_t *uart;

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

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

	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	/* uart is not mappable to any region */
	return dev_isNotMappable;
}


static int uart_init(unsigned int minor)
{
	uart_t *uart;

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	/* UART Clock Controller configuration */
	if (uart_common.clkInit == 0) {
		uart_initCtrlClock();
		uart_common.clkInit = 1;
	}


	if (_zynq_setAmbaClk(info[minor].clk, clk_enable) < 0)
		return -EINVAL;

	if (uart_setPin(info[minor].rxPin) < 0 || uart_setPin(info[minor].txPin) < 0)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	uart->irq = info[minor].irq;
	uart->clk = info[minor].clk;
	uart->base = info[minor].base;

	/* Skip controller initialization if it has been already done by hal */
	if (!(*(uart->base + cr) & (1 << 4 | 1 << 2))) {
		/* Reset RX & TX */
		*(uart->base + cr) = 0x3;

		/* Uart Mode Register
			* normal mode, 1 stop bit, no parity, 8 bits, uart_ref_clk as source clock
			* PAR = 0x4 */
		*(uart->base + mr) = (*(uart->base + mr) & ~0x000003ff) | 0x00000020;

		/* Disable TX and RX */
		*(uart->base + cr) = (*(uart->base + cr) & ~0x000001ff) | 0x00000028;

		uart_calcBaudarate(uart, UART_BAUDRATE);

		/* Uart Control Register
			* TXEN = 0x1; RXEN = 0x1; TXRES = 0x1; RXRES = 0x1 */
		*(uart->base + cr) = (*(uart->base + cr) & ~0x000001ff) | 0x00000017;
	}

	/* Enable RX FIFO trigger */
	*(uart->base + ier) |= 0x1;
	/* Set trigger level, range: 1-63 */
	*(uart->base + rxwm) = 1;

	lib_printf("\ndev/uart: Initializing uart(%d.%d)", DEV_UART, minor);
	hal_irqinst(info[minor].irq, uart_irqHandler, (void *)uart);

	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_handler_t h = {
		.init = uart_init,
		.done = uart_done,
		.read = uart_read,
		.write = uart_safeWrite,
		.sync = uart_sync,
		.map = uart_map,
	};

	devs_register(DEV_UART, UARTS_MAX_CNT, &h);
}
