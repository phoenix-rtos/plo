/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL Serial driver
 *
 * Copyright 2018, 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/lib.h>
#include <devices/devs.h>


#define UARTS_MAX_CNT 8
#define BUFFER_SIZE   0x20
#define UART_REF_CLK  20000000


typedef struct {
	volatile u32 *base;
	unsigned int irq;
	u16 clk;

	u8 dataRx[BUFFER_SIZE];
	cbuffer_t cbuffRx;

	u8 dataTx[BUFFER_SIZE];
	cbuffer_t cbuffTx;
} uart_t;


typedef struct {
	int ioctl;
	char val;
} uart_ioctl_t;


enum { urxd = 0, utxd = 16, ucr1 = 32, ucr2, ucr3, ucr4, ufcr, usr1, usr2,
	uesc, utim, ubir, ubmr, ubrc, onems, uts, umcr };


static const uart_ioctl_t uart_mux[UARTS_MAX_CNT][4] = {
	{ { mux_uart1_cts, 0 }, { mux_uart1_rts, 0 }, { mux_uart1_rx, 0 }, { mux_uart1_tx, 0 } },
	{ { mux_uart2_cts, 0 }, { mux_uart2_rts, 0 }, { mux_uart2_rx, 0 }, { mux_uart2_tx, 0 } },
	{ { mux_uart3_cts, 0 }, { mux_uart3_rts, 0 }, { mux_uart3_rx, 0 }, { mux_uart3_tx, 0 } },
	{ { mux_lcd_hsync, 2 }, { mux_lcd_vsync, 2 }, { mux_uart4_rx, 0 }, { mux_uart4_tx, 0 } },
	{ { mux_gpio1_09, 8 }, { mux_gpio1_08, 8 }, { mux_uart5_rx, 0 }, { mux_uart5_tx, 0 } },
	{ { mux_enet1_tx1, 1 }, { mux_enet1_txen, 1 }, { mux_enet2_rx1, 1 }, { mux_enet2_rx0, 1 } },
	{ { mux_lcd_d6, 1 }, { mux_lcd_d7, 1 }, { mux_lcd_d17, 1 }, { mux_lcd_d16, 1 } },
	{ { mux_lcd_d4, 1 }, { mux_lcd_d5, 1 }, { mux_lcd_d21, 1 }, { mux_lcd_d20, 1 } },
};


static const uart_ioctl_t uart_isel[UARTS_MAX_CNT][2] = {
	{ { isel_uart1_rts, 3 }, { isel_uart1_rx, 3 } },
	{ { isel_uart2_rts, 1 }, { isel_uart2_rx, 1 } },
	{ { isel_uart3_rts, 1 }, { isel_uart3_rx, 1 } },
	{ { isel_uart4_rts, 3 }, { isel_uart4_rx, 1 } },
	{ { isel_uart5_rts, 1 }, { isel_uart5_rx, 7 } },
	{ { isel_uart6_rts, 3 }, { isel_uart6_rx, 2 } },
	{ { isel_uart7_rts, 3 }, { isel_uart7_rx, 3 } },
	{ { isel_uart8_rts, 3 }, { isel_uart8_rx, 3 } },
};


static const struct {
	volatile u32 *base;
	unsigned int irq;
	u32 clk;
} info[UARTS_MAX_CNT] = {
	{ (void *)0x02020000, 58, clk_uart1 },
	{ (void *)0x021E8000, 59, clk_uart2 },
	{ (void *)0x021EC000, 60, clk_uart3 },
	{ (void *)0x021F0000, 61, clk_uart4 },
	{ (void *)0x021F4000, 62, clk_uart5 },
	{ (void *)0x021FC000, 49, clk_uart6 },
	{ (void *)0x02018000, 71, clk_uart7 },
	{ (void *)0x02288000, 72, clk_uart8 }
};


struct {
	uart_t uarts[UARTS_MAX_CNT];
	int clkInit;
} uart_common;


static inline void uart_rxData(uart_t *uart)
{
	char c;

	/* Keep getting data until fifo is not empty */
	while (*(uart->base + usr2) & 0x1) {
		c = *(uart->base + urxd);
		lib_cbufWrite(&uart->cbuffRx, &c, 1);
	}
}


static inline void uart_txData(uart_t *uart)
{
	char c;

	while (!lib_cbufEmpty(&uart->cbuffTx)) {
		if (!(*(uart->base + uts) & (0x1 << 4))) {
			lib_cbufRead(&uart->cbuffTx, &c, 1);
			*(uart->base + utxd) = c;
		}
	}

	*(uart->base + ucr1) &= ~(1 << 6);
}


static int uart_irqHandler(unsigned int n, void *data)
{
	u32 usr;
	uart_t *uart = (uart_t *)data;

	if (uart == NULL)
		return 0;

	usr = *(uart->base + usr2);

	/* Rx fifo trigger irq */
	if (usr & 0x1)
		uart_rxData(uart);

	/* Tx fifo is empty */
	if (usr & (1 << 14) && !lib_cbufEmpty(&uart->cbuffTx))
		uart_txData(uart);

	return 0;
}


static void uart_padsInit(unsigned int minor)
{
	int i;

	for (i = 0; i < 4; i++)
		imx6ull_setIOmux(uart_mux[minor][i].ioctl, 0, uart_mux[minor][i].val);

	for (i = 0; i < 2; i++)
		imx6ull_setIOisel(uart_isel[minor][i].ioctl, uart_isel[minor][i].val);
}


/* According to Reference Manual:
 * baud_rate = ref_freq / (16 * (UBMR + 1) / (UBIR + 1)) */
static int uart_calcBaudrate(uart_t *uart, int baudrate)
{
	u32 max, div = 1;
	u32 tNum, tDen = 0;
	u32 calcBaudrate, diff;

	u32 num = UART_REF_CLK;
	u32 den = 16 * baudrate;

	/* Get GCD for ref_freq and (16 * baud_rate) */
	while (den != 0) {
		div = den;
		den = num % den;
		num = div;
	}

	/* ref_freq / (16 * baudrate) = (UBMR + 1) / (UBIR + 1) */
	num = UART_REF_CLK / div;
	den = (16 * baudrate) / div;

	/* Range of numerator & denominator - 1 ~ 0xffff */
	if ((num > 0xffff) || (den > 0xffff)) {
		tNum = (num - 1) / 0xffff + 1;
		tDen = (den - 1) / 0xffff + 1;
		max = tNum > tDen ? tNum : tDen;

		num /= max;
		den /= max;

		if (!num)
			num = 1;

		if (!den)
			den = 1;
	}

	calcBaudrate = ((UART_REF_CLK >> 4) * den) / num;
	diff = (calcBaudrate >= baudrate) ? (calcBaudrate - baudrate) : (baudrate - calcBaudrate);

	if (((diff * 100) / baudrate) > 3)
		return -1;

	/* Disable automatic detection */
	*(uart->base + ucr1) &= ~(1 << 14);

	*(uart->base + ubir) = den - 1;
	*(uart->base + ubmr) = num - 1;

	return 0;
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

	/* Enable TX FIFO empty irq */
	*(uart->base + ucr1) |= 1 << 6;
	hal_interruptsEnable(info[minor].irq);

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

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	while (!lib_cbufEmpty(&uart->cbuffTx))
		;

	/* Wait until TxFIFO is empty */
	while (!(*(uart->base + uts) & (1 << 6)))
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

	/* Controller reset */
	*(uart->base + ucr2) &= ~0x1;

	/* Wait until software reset is inactive */
	while (*(uart->base + uts) & 0x1)
		;

	/* Disable uart */
	*(uart->base + ucr2) = 0;

	hal_interruptsSet(uart->irq, NULL, NULL);

	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	/* Uart is not mappable to any region */
	return dev_isNotMappable;
}


static int uart_init(unsigned int minor)
{
	uart_t *uart;

	if (minor >= UARTS_MAX_CNT)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	uart->irq = info[minor].irq;
	uart->clk = info[minor].clk;
	uart->base = info[minor].base;

	lib_cbufInit(&uart->cbuffTx, uart->dataTx, BUFFER_SIZE);
	lib_cbufInit(&uart->cbuffRx, uart->dataRx, BUFFER_SIZE);

	if (!(*(uart->base + ucr1) & 0x1)) {
		imx6ull_setDevClock(uart->clk, 3);
		uart_padsInit(minor);

		/* Controller reset */
		*(uart->base + ucr2) &= ~0x1;

		/* Wait until software reset is inactive */
		while (*(uart->base + uts) & 0x1)
			;

		/* Set TX & RX FIFO watermark, DCE mode */
		*(uart->base + ufcr) = (0x04 << 10) | (0 << 6) | (0x1);

		/* Set Reference Frequency Divider = 4 */
		*(uart->base + ufcr) &= ~(0b111 << 7);
		*(uart->base + ufcr) |= 0b010 << 7;

		/* CS8 */
		*(uart->base + ucr2) |= (1 << 6);

		/* Soft reset, tx&rx enable, 8bit transmit */
		*(uart->base + ucr2) = 0x4027;

		/* Disable parity */
		*(uart->base + ucr2) &= ~((1 << 8) | (1 << 7));
		/* Sends 1 stop bit */
		*(uart->base + ucr2) &= ~(1 << 6);
	}

	uart_calcBaudrate(uart, 115200);

	/* 32 characters in the RxFIFO */
	*(uart->base + ucr4) = 0x8000;
	/* Enable uart and rx ready interrupt */
	*(uart->base + ucr1) = 0x0201;
	/* Data Terminal Ready Delta Enable, Ring Indicator, Data Carrier Detect,
	 * Data Set Ready */
	*(uart->base + ucr3) = 0x704;

	hal_interruptsSet(info[minor].irq, uart_irqHandler, (void *)uart);
	lib_printf("\ndev/uart: Initializing uart(%d.%d)", DEV_UART, minor);

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
