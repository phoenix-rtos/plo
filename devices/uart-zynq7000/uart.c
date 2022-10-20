/*
 * Phoenix-RTOS
 *
 * Operating system loader
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
#include <lib/lib.h>
#include <devices/devs.h>

#include <board_config.h>


#define MAX_TXRX_FIFO_SIZE 0x40
#define BUFFER_SIZE        0x200

typedef struct {
	volatile u32 *base;
	unsigned int irq;
	u16 clk;

	u8 dataRx[BUFFER_SIZE];
	cbuffer_t cbuffRx;

	u8 dataTx[BUFFER_SIZE];
	cbuffer_t cbuffTx;
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
	unsigned int irq;
	u16 clk;
	u16 rxPin;
	u16 txPin;
} info[UARTS_MAX_CNT] = {
	{ UART0_BASE_ADDR, UART0_IRQ, UART0_CLK, UART0_RX, UART0_TX },
	{ UART1_BASE_ADDR, UART1_IRQ, UART1_CLK, UART1_RX, UART1_TX }
};


static inline void uart_rxData(uart_t *uart)
{
	char c;
	/* Keep getting data until fifo is not empty */
	while (!(*(uart->base + sr) & (0x1 << 1))) {
		c = *(uart->base + fifo) & 0xff;
		lib_cbufWrite(&uart->cbuffRx, &c, 1);
	}

	*(uart->base + isr) = 0x1;
}


static inline void uart_txData(uart_t *uart)
{
	char c;

	/* Keep filling until fifo is not full */
	while (!lib_cbufEmpty(&uart->cbuffTx)) {
		if (!(*(uart->base + sr) & (0x1 << 4))) {
			lib_cbufRead(&uart->cbuffTx, &c, 1);
			*(uart->base + fifo) = c;
		}
	}
}


static int uart_irqHandler(unsigned int n, void *data)
{
	u32 st;
	uart_t *uart = (uart_t *)data;

	if (uart == NULL)
		return 0;

	st = *(uart->base + isr);

	/* RX FIFO trigger irq */
	if (st & 0x1)
		uart_rxData(uart);

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

	switch (pin) {
		case UART0_RX:
		case UART1_RX:
			ctl.triEnable = 1;
			break;

		case UART0_TX:
		case UART1_TX:
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


/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	size_t res;
	uart_t *uart;
	time_t start;

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	start = hal_timerGet();
	while (lib_cbufEmpty(&uart->cbuffRx)) {
		if ((hal_timerGet() - start) >= timeout)
			return -ETIME;
	}

	hal_interruptsDisable(info[minor].irq);
	res = lib_cbufRead(&uart->cbuffRx, buff, len);
	hal_interruptsEnable(info[minor].irq);

	return res;
}


static ssize_t uart_write(unsigned int minor, const void *buff, size_t len)
{
	size_t res;
	uart_t *uart;

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	while (uart->cbuffTx.full)
		;

	hal_interruptsDisable(info[minor].irq);
	res = lib_cbufWrite(&uart->cbuffTx, buff, len);
	hal_interruptsEnable(info[minor].irq);

	if (res > 0) {
		uart_txData(uart);
	}

	return res;
}


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	size_t l;

	for (l = 0; l < len; l += res) {
		if ((res = uart_write(minor, (const char *)buff + l, len - l)) < 0)
			return -ENXIO;
	}

	return len;
}


static int uart_sync(unsigned int minor)
{
	uart_t *uart;
	time_t start;

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	while (!lib_cbufEmpty(&uart->cbuffTx))
		;

	/* Wait until TxFIFO is empty */
	while (!(*(uart->base + sr) & (0x1 << 3)))
		;

	/* Although status register indicates that TxFIFO is empty, data is transmitted.
	 * To not lose any data, wait 3 ms to clean up the fifo */
	start = hal_timerGet();
	while (hal_timerGet() - start < 3)
		;

	return EOK;
}


static int uart_done(unsigned int minor)
{
	int res;
	uart_t *uart;

	if ((res = uart_sync(minor)) < 0)
		return res;

	uart = &uart_common.uarts[minor];

	/* Disable interrupts */
	*(uart->base + idr) = 0xfff;

	/* Disable RX & TX */
	*(uart->base + cr) |= (1 << 5) | (1 << 3);
	/* Reset RX & TX */
	*(uart->base + cr) |= 0x3;

	/* Clear status flags */
	*(uart->base + isr) = 0xfff;

	hal_interruptsSet(uart->irq, NULL, NULL);
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

	uart = &uart_common.uarts[minor];

	uart->irq = info[minor].irq;
	uart->clk = info[minor].clk;
	uart->base = info[minor].base;

	lib_cbufInit(&uart->cbuffTx, uart->dataTx, BUFFER_SIZE);
	lib_cbufInit(&uart->cbuffRx, uart->dataRx, BUFFER_SIZE);

	/* Skip controller initialization if it has been already done by hal */
	if (!(*(uart->base + cr) & (1 << 4 | 1 << 2))) {
		if (_zynq_setAmbaClk(info[minor].clk, clk_enable) < 0)
			return -EINVAL;

		if (uart_setPin(info[minor].rxPin) < 0 || uart_setPin(info[minor].txPin) < 0)
			return -EINVAL;

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
	hal_interruptsSet(info[minor].irq, uart_irqHandler, (void *)uart);

	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_handler_t h = {
		.init = uart_init,
		.done = uart_done,
		.read = uart_read,
		.write = uart_safeWrite,
		.erase = NULL,
		.sync = uart_sync,
		.map = uart_map,
	};

	devs_register(DEV_UART, UARTS_MAX_CNT, &h);
}
