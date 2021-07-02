/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * UART 16550
 *
 * Copyright 2012-2015, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2008 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Pawel Kolodziej, Julia Kosowska, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>

#include "uarthw.h"
#include "uart-16550.h"


typedef struct {
	unsigned char hwctx[SIZE_UARTHW_CTX];
	unsigned char active;

	/* Receiver */
	unsigned char rx[SIZE_RX];
	volatile unsigned int rh;
	volatile unsigned int rt;

	/* Transmitter */
	unsigned char tx[SIZE_TX];
	volatile unsigned int th;
	volatile unsigned int tt;
	volatile unsigned char tf;
} uart_t;


const baud_t bauds[] = {
	{ 50,      2304,  0 }, /*  0 - BPS_50 */
	{ 75,      1536,  0 }, /*  1 - BPS_75 */
	{ 110,     1047,  0 }, /*  2 - BPS_110 */
	{ 135,     857,   0 }, /*  3 - BPS_135 */
	{ 150,     768,   0 }, /*  4 - BPS_150 */
	{ 300,     384,   0 }, /*  5 - BPS_300 */
	{ 600,     192,   0 }, /*  6 - BPS_600 */
	{ 1200,    96,    0 }, /*  7 - BPS_1200 */
	{ 1800,    64,    0 }, /*  8 - BPS_1800 */
	{ 2000,    58,    0 }, /*  9 - BPS_2000 */
	{ 2400,    48,    0 }, /* 10 - BPS_2400 */
	{ 3600,    32,    0 }, /* 11 - BPS_3600 */
	{ 4800,    24,    0 }, /* 12 - BPS_4800 */
	{ 7200,    16,    0 }, /* 13 - BPS_7200 */
	{ 9600,    12,    0 }, /* 14 - BPS_9600 */
	{ 19200,   6,     0 }, /* 15 - BPS_19200 */
	{ 28800,   4,     0 }, /* 16 - BPS_28800 */
	{ 31250,   4,     1 }, /* 17 - BPS_31250 */
	{ 38400,   3,     0 }, /* 18 - BPS_38400 */
	{ 57600,   2,     0 }, /* 19 - BPS_57600 */
	{ 115200,  1,     0 }, /* 20 - BPS_115200 */
	{ 230400,  32770, 2 }, /* 21 - BPS_230400 */
	{ 460800,  32769, 2 }, /* 22 - BPS_460800 */
	{ 921600,  49153, 6 }, /* 23 - BPS_921600 */
	{ 1500000, 8193,  8 }  /* 24 - BPS_1500000 */
};


struct {
	uart_t uarts[UART_MAX_CNT];
} uart_common;


/* Returns UART instance */
static uart_t *uart_get(unsigned int minor)
{
	uart_t *uart;

	if (minor >= UART_MAX_CNT)
		return NULL;
	uart = &uart_common.uarts[minor];

	return (uart->active) ? uart : NULL;
}


static int uart_isr(unsigned int irq, void *arg)
{
	uart_t *uart = (uart_t *)arg;
	unsigned char iir;

	for (;;) {
		if ((iir = uarthw_read(uart->hwctx, IIR)) & 0x01)
			break;

		/* Receive */
		if (iir & 0x04) {
			while (uarthw_read(uart->hwctx, LSR) & 0x01) {
				uart->rx[uart->rh] = uarthw_read(uart->hwctx, RBR);
				if ((uart->rh = (++uart->rh % SIZE_RX)) == uart->rt)
					uart->rt = (++uart->rt % SIZE_RX);
			}
		}

		/* Transmit */
		if (iir & 0x02) {
			if ((uart->th = (++uart->th % SIZE_TX)) != uart->tt)
				uarthw_write(uart->hwctx, THR, uart->tx[uart->th]);
			else
				uarthw_write(uart->hwctx, IER, 0x01);

			/* TX buffer not full */
			uart->tf = 0;
		}
	}

	return EOK;
}


static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	uart_t *uart;
	time_t start;
	unsigned int l;

	if ((uart = uart_get(minor)) == NULL)
		return -EINVAL;

	if (!len)
		return 0;

	start = hal_timerGet();
	while (uart->rh == uart->rt) {
		if (hal_timerGet() - start >= timeout)
			return -ETIME;
	}

	hal_interruptsDisable();

	l = (uart->rh > uart->rt) ? min(uart->rh - uart->rt, len) : min(SIZE_RX - uart->rt, len);
	hal_memcpy(buff, &uart->rx[uart->rt], l);

	if ((l < len) && (uart->rh < uart->rt)) {
		hal_memcpy(buff + l, uart->rx, min(len - l, uart->rh));
		l += min(len - l, uart->rh);
	}
	uart->rt = (uart->rt + l) % SIZE_RX;

	hal_interruptsEnable();

	return l;
}


static ssize_t uart_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	uart_t *uart;
	unsigned int l;

	if ((uart = uart_get(minor)) == NULL)
		return -EINVAL;

	if (!len)
		return 0;

	/* Wait while TX buffer is full */
	while (uart->tf);

	hal_interruptsDisable();

	l = (uart->th > uart->tt) ? min(uart->th - uart->tt, len) : min(SIZE_TX - uart->tt, len);
	hal_memcpy(&uart->tx[uart->tt], buff, l);

	if ((l < len) && (uart->tt >= uart->th)) {
		hal_memcpy(uart->tx, buff + l, min(len - l, uart->th));
		l += min(len - l, uart->th);
	}

	/* Initialize sending process if TX buffer was empty */
	if (uart->tt == uart->th) {
		uarthw_write(uart->hwctx, THR, uart->tx[uart->th]);
		uarthw_write(uart->hwctx, IER, 0x03);
	}

	/* Check if TX buffer is full */
	if ((uart->tt = (uart->tt + l) % SIZE_TX) == uart->th)
		uart->tf = 1;

	hal_interruptsEnable();

	return l;
}


static int _uart_sync(uart_t *uart)
{
	/* Wait until TX buffer is empty */
	while (uart->tf);
	while (uart->tt != uart->th);

	return EOK;
}


static int uart_sync(unsigned int minor)
{
	uart_t *uart;

	if ((uart = uart_get(minor)) == NULL)
		return -EINVAL;

	return _uart_sync(uart);
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	uart_t *uart;

	if ((uart = uart_get(minor)) == NULL)
		return -EINVAL;

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	return dev_isNotMappable;
}


static int uart_done(unsigned int minor)
{
	uart_t *uart;
	int err;

	if ((uart = uart_get(minor)) == NULL)
		return -EINVAL;

	if ((err = _uart_sync(uart)) < 0)
		return err;

	/* Uninstall interrupt handler */
	hal_interruptsSet(uarthw_irq(uart->hwctx), NULL, NULL);

	return EOK;
}


static int uart_init(unsigned int minor)
{
	unsigned int baud = BPS_115200;
	uart_t *uart;
	int err;

	if (minor >= UART_MAX_CNT)
		return -EINVAL;
	uart = &uart_common.uarts[minor];

	/* Initialize UART hardware context */
	if ((err = uarthw_init(minor, uart->hwctx, &baud)) < 0)
		return err;

	/* Detect device presence */
	if (uarthw_read(uart->hwctx, IIR) == 0xff)
		return -ENOENT;
	uart->active = 1;

	/* Set interrupt handler */
	hal_interruptsSet(uarthw_irq(uart->hwctx), uart_isr, uart);

	/* Set speed */
	uarthw_write(uart->hwctx, LCR, 0x80);
	uarthw_write(uart->hwctx, DLL, bauds[baud].divisor);
	uarthw_write(uart->hwctx, DLM, bauds[baud].divisor >> 8);

	/* Set data format */
	uarthw_write(uart->hwctx, LCR, 0x03);

	/* Enable and configure FIFOs */
	uarthw_write(uart->hwctx, FCR, 0xa7);

	/* Enable hardware interrupts */
	uarthw_write(uart->hwctx, MCR, 0x08);

	/* Set interrupt mask */
	uarthw_write(uart->hwctx, IER, 0x01);

	return EOK;
}


__attribute__((constructor)) static void uart_register(void)
{
	static const dev_handler_t h = {
		.read = uart_read,
		.write = uart_write,
		.sync = uart_sync,
		.map = uart_map,
		.done = uart_done,
		.init = uart_init
	};

	devs_register(DEV_UART, UART_MAX_CNT, &h);
}
