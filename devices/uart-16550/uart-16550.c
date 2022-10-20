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


/* clang-format off */
const baud_t bauds[] = {
	{ 50,      2304,  0 }, /*  0 - bps_50 */
	{ 75,      1536,  0 }, /*  1 - bps_75 */
	{ 110,     1047,  0 }, /*  2 - bps_110 */
	{ 135,     857,   0 }, /*  3 - bps_135 */
	{ 150,     768,   0 }, /*  4 - bps_150 */
	{ 300,     384,   0 }, /*  5 - bps_300 */
	{ 600,     192,   0 }, /*  6 - bps_600 */
	{ 1200,    96,    0 }, /*  7 - bps_1200 */
	{ 1800,    64,    0 }, /*  8 - bps_1800 */
	{ 2000,    58,    0 }, /*  9 - bps_2000 */
	{ 2400,    48,    0 }, /* 10 - bps_2400 */
	{ 3600,    32,    0 }, /* 11 - bps_3600 */
	{ 4800,    24,    0 }, /* 12 - bps_4800 */
	{ 7200,    16,    0 }, /* 13 - bps_7200 */
	{ 9600,    12,    0 }, /* 14 - bps_9600 */
	{ 19200,   6,     0 }, /* 15 - bps_19200 */
	{ 28800,   4,     0 }, /* 16 - bps_28800 */
	{ 31250,   4,     1 }, /* 17 - bps_31250 */
	{ 38400,   3,     0 }, /* 18 - bps_38400 */
	{ 57600,   2,     0 }, /* 19 - bps_57600 */
	{ 115200,  1,     0 }, /* 20 - bps_115200 */
	{ 230400,  32770, 2 }, /* 21 - bps_230400 */
	{ 460800,  32769, 2 }, /* 22 - bps_460800 */
	{ 921600,  49153, 6 }, /* 23 - bps_921600 */
	{ 1500000, 8193,  8 }  /* 24 - bps_1500000 */
};
/* clang-format on */


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
	unsigned char isr;

	for (;;) {
		if ((isr = uarthw_read(uart->hwctx, iir)) & 0x01)
			break;

		/* Receive */
		if (isr & 0x04) {
			while (uarthw_read(uart->hwctx, lsr) & 0x01) {
				uart->rx[uart->rh] = uarthw_read(uart->hwctx, rbr);
				if ((uart->rh = (++uart->rh % SIZE_RX)) == uart->rt)
					uart->rt = (++uart->rt % SIZE_RX);
			}
		}

		/* Transmit */
		if (isr & 0x02) {
			if ((uart->th = (++uart->th % SIZE_TX)) != uart->tt)
				uarthw_write(uart->hwctx, thr, uart->tx[uart->th]);
			else
				uarthw_write(uart->hwctx, ier, 0x01);

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

	hal_interruptsDisableAll();

	l = (uart->rh > uart->rt) ? min(uart->rh - uart->rt, len) : min(SIZE_RX - uart->rt, len);
	hal_memcpy(buff, &uart->rx[uart->rt], l);

	if ((l < len) && (uart->rh < uart->rt)) {
		hal_memcpy((char *)buff + l, uart->rx, min(len - l, uart->rh));
		l += min(len - l, uart->rh);
	}
	uart->rt = (uart->rt + l) % SIZE_RX;

	hal_interruptsEnableAll();

	return l;
}


static ssize_t uart_write(unsigned int minor, const void *buff, size_t len)
{
	uart_t *uart;
	unsigned int l;

	if ((uart = uart_get(minor)) == NULL)
		return -EINVAL;

	if (!len)
		return 0;

	/* Wait while TX buffer is full */
	while (uart->tf)
		;

	hal_interruptsDisableAll();

	l = (uart->th > uart->tt) ? min(uart->th - uart->tt, len) : min(SIZE_TX - uart->tt, len);
	hal_memcpy(&uart->tx[uart->tt], buff, l);

	if ((l < len) && (uart->tt >= uart->th)) {
		hal_memcpy(uart->tx, (const char *)buff + l, min(len - l, uart->th));
		l += min(len - l, uart->th);
	}

	/* Initialize sending process if TX buffer was empty */
	if (uart->tt == uart->th) {
		uarthw_write(uart->hwctx, thr, uart->tx[uart->th]);
		uarthw_write(uart->hwctx, ier, 0x03);
	}

	/* Check if TX buffer is full */
	if ((uart->tt = (uart->tt + l) % SIZE_TX) == uart->th)
		uart->tf = 1;

	hal_interruptsEnableAll();

	return l;
}


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	size_t l;

	for (l = 0; l < len; l += res) {
		if ((res = uart_write(minor, (const char *)buff + l, len - l)) < 0)
			return res;
	}

	return len;
}


static int _uart_sync(uart_t *uart)
{
	/* Wait until TX buffer is empty */
	while (uart->tf)
		;
	while (uart->tt != uart->th)
		;

	/* Wait for transmission to complete */
	while (!(uarthw_read(uart->hwctx, lsr) & 0x40))
		;

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

	/* Disable controller interrupts */
	uarthw_write(uart->hwctx, ier, 0x00);
	uarthw_write(uart->hwctx, mcr, 0x00);

	/* Flush and disable FIFOs */
	uarthw_write(uart->hwctx, fcr, 0x06);

	/* Uninstall interrupt handler */
	hal_interruptsSet(uarthw_irq(uart->hwctx), NULL, NULL);

	return EOK;
}


static int uart_init(unsigned int minor)
{
	unsigned int baud = bps_115200;
	uart_t *uart;
	int err;

	if (minor >= UART_MAX_CNT)
		return -EINVAL;
	uart = &uart_common.uarts[minor];

	/* Initialize UART hardware context */
	if ((err = uarthw_init(minor, uart->hwctx, &baud)) < 0)
		return err;

	/* Detect device presence */
	if (uarthw_read(uart->hwctx, iir) == 0xff)
		return -ENODEV;
	uart->active = 1;

	/* Set interrupt handler */
	hal_interruptsSet(uarthw_irq(uart->hwctx), uart_isr, uart);

	/* Set speed */
	uarthw_write(uart->hwctx, lcr, 0x80);
	uarthw_write(uart->hwctx, dll, bauds[baud].divisor);
	uarthw_write(uart->hwctx, dlm, bauds[baud].divisor >> 8);

	/* Set data format */
	uarthw_write(uart->hwctx, lcr, 0x03);

	/* Enable and configure FIFOs */
	uarthw_write(uart->hwctx, fcr, 0xa7);

	/* Enable hardware interrupts */
	uarthw_write(uart->hwctx, mcr, 0x08);

	/* Set interrupt mask */
	uarthw_write(uart->hwctx, ier, 0x01);

	return EOK;
}


__attribute__((constructor)) static void uart_register(void)
{
	static const dev_handler_t h = {
		.read = uart_read,
		.write = uart_safeWrite,
		.erase = NULL,
		.sync = uart_sync,
		.map = uart_map,
		.done = uart_done,
		.init = uart_init
	};

	devs_register(DEV_UART, UART_MAX_CNT, &h);
}
