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

#define UARTS_MAX_CNT_      1
#define UART0_BASE_ADDR_    0x09000000
#define UART0_IRQ_          33 /* GIC SPI 1: 32 + 1 = 33 */

#ifndef DEV_UART
#define DEV_UART           1
#endif

#define MAX_TXRX_FIFO_SIZE 0x40
#define BUFFER_SIZE        0x200

typedef struct {
	volatile u32 *base;
	unsigned int irq;

	u8 dataRx[BUFFER_SIZE];
	cbuffer_t cbuffRx;

	u8 dataTx[BUFFER_SIZE];
	cbuffer_t cbuffTx;
} uart_t;


enum {
	dr = 0x00,
	rsrecr = 0x01,
	fr = 0x06,
	ilpr = 0x08,
	ibrd = 0x09,
	fbrd = 0x0a,
	lcr_h = 0x0b,
	cr = 0x0c,
	ifls = 0x0d,
	imsc = 0x0e,
	ris = 0x0f,
	mis = 0x10,
	icr = 0x11,
};

#define PL011_FR_TXFF   (1 << 5)
#define PL011_FR_RXFE   (1 << 4)
#define PL011_IMSC_RXIM (1 << 4)
#define PL011_ICR_RXIC  (1 << 4)

struct {
	uart_t uarts[UARTS_MAX_CNT_];
} uart_common;

const struct {
	volatile u32 *base;
	unsigned int irq;
} info[UARTS_MAX_CNT_] = {
	{ (volatile u32 *)UART0_BASE_ADDR_, UART0_IRQ_ },
};

static inline void uart_rxData(uart_t *uart)
{
	char c;
	/* Keep getting data until fifo is not empty */
	while (!(*(uart->base + fr) & PL011_FR_RXFE)) {
		c = *(uart->base + dr) & 0xff;
		lib_cbufWrite(&uart->cbuffRx, &c, 1);
	}

	*(uart->base + icr) = PL011_ICR_RXIC;
}


static inline void uart_txData(uart_t *uart)
{
	char c;

	/* Keep filling until fifo is not full */
	while (!lib_cbufEmpty(&uart->cbuffTx)) {
		if (!(*(uart->base + fr) & PL011_FR_TXFF)) {
			lib_cbufRead(&uart->cbuffTx, &c, 1);
			*(uart->base + dr) = c;
		} else {
			break;
		}
	}
}


static int uart_irqHandler(unsigned int n, void *data)
{
	u32 st;
	uart_t *uart = (uart_t *)data;

	if (uart == NULL)
		return 0;

	st = *(uart->base + mis);

	/* RX FIFO trigger interrupt check */
	if (st & PL011_IMSC_RXIM)
		uart_rxData(uart);

	return 0;
}

/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	size_t res;
	uart_t *uart;
	time_t start;

	if (minor >= UARTS_MAX_CNT_)
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

	if (minor >= UARTS_MAX_CNT_)
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

	if (minor >= UARTS_MAX_CNT_)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	while (!lib_cbufEmpty(&uart->cbuffTx))
		;

	/* Wait until TxFIFO is empty */
	while (*(uart->base + fr) & PL011_FR_TXFF)
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
	*(uart->base + imsc) = 0x0;

	/* Disable UART entirely (Control Register = 0) */
	*(uart->base + cr) = 0x0;

	/* Clear pending interrupts */
	*(uart->base + icr) = 0x7ff;

	hal_interruptsSet(uart->irq, NULL, NULL);

	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor >= UARTS_MAX_CNT_)
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

	if (minor >= UARTS_MAX_CNT_)
		return -EINVAL;

	uart = &uart_common.uarts[minor];

	uart->irq = info[minor].irq;
	uart->base = info[minor].base;

	lib_cbufInit(&uart->cbuffTx, uart->dataTx, BUFFER_SIZE);
	lib_cbufInit(&uart->cbuffRx, uart->dataRx, BUFFER_SIZE);

	/* Check if the UART is already configured/running */
	if (!(*(uart->base + cr) & 0x1)) {
		/* PL011 Initialization Flow */
		*(uart->base + cr) = 0x0;     /* Disable current settings */
		*(uart->base + icr) = 0x7ff;  /* Clear interrupt vectors */

		/* Set parameters on Line Control High Register:
		 * WLEN = 8 bits (0x60), Enable FIFOs (0x10) */
		*(uart->base + lcr_h) = 0x70; 

		*(uart->base + ifls) = 0x0;

		/* Re-enable UART, Transmit Enable (TXE), Receive Enable (RXE)
		 * CR Bit 0 = UARTEN, Bit 8 = TXE, Bit 9 = RXE */
		*(uart->base + cr) = (1 << 0) | (1 << 8) | (1 << 9);
	}

	/* Unmask the Receive Interrupt inside PL011 */
	*(uart->base + imsc) |= PL011_IMSC_RXIM;

	lib_printf("\ndev/uart: Initializing uart(%d.%d)", DEV_UART, minor);
	hal_interruptsSet(info[minor].irq, uart_irqHandler, (void *)uart);

	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_ops_t opsUartPL011 = {
		.read = uart_read,
		.write = uart_safeWrite,
		.erase = NULL,
		.sync = uart_sync,
		.map = uart_map,
	};

	static const dev_t devUartPL011 = {
		.name = "uart-pl011",
		.init = uart_init,
		.done = uart_done,
		.ops = &opsUartPL011,
	};

	devs_register(DEV_UART, UARTS_MAX_CNT_, &devUartPL011);
}
