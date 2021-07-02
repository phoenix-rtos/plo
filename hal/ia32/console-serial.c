/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Serial console
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Michał Mirosław, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


/* UART registers */
enum {
	RBR = 0, /* Receiver Buffer Register */
	THR = 0, /* Transmitter Holding Register */
	DLL = 0, /* Divisor Latch LSB */
	IER = 1, /* Interrupt Enable Register */
	DLM = 1, /* Divisor Latch MSB */
	IIR = 2, /* Interrupt Identification Register */
	FCR = 2, /* FIFO Control Register */
	LCR = 3, /* Line Control Register */
	MCR = 4, /* Modem Control Register */
	LSR = 5, /* Line Status Register */
	MSR = 6, /* Modem Status Register */
	SPR = 7, /* Scratch Pad Register */
};


struct {
	void *base;
} halconsole_common;


static unsigned char console_read(void *base, unsigned int reg)
{
	unsigned int addr = (unsigned int)base;

	/* Read from IO-port */
	if (addr & 0x1) {
		addr &= ~0x3;
		addr += reg;

		return hal_inb((void *)addr);
	}

	/* Read from memory */
	addr &= ~0xf;
	addr += reg;

	return *(volatile unsigned char *)addr;
}


static void console_write(void *base, unsigned int reg, unsigned char val)
{
	unsigned int addr = (unsigned int)base;

	/* Write to IO-port */
	if (addr & 0x1) {
		addr &= ~0x3;
		addr += reg;

		hal_outb((void *)addr, val);
		return;
	}

	/* Write to memory */
	addr &= ~0xf;
	addr += reg;

	*(volatile unsigned char *)addr = val;
}


void hal_consolePrint(const char *s)
{
	for (; *s; s++) {
		/* Wait until TX fifo is empty */
		while (!(console_read(halconsole_common.base, LSR) & 0x20));
		console_write(halconsole_common.base, THR, *s);
	}
}


void hal_consoleInit(void)
{
	unsigned int divisor = 115200 / UART_BAUDRATE;

	halconsole_common.base = (void *)(UART_COM1 | 0x1);

	/* Set speed */
	console_write(halconsole_common.base, LCR, 0x80);
	console_write(halconsole_common.base, DLL, divisor);
	console_write(halconsole_common.base, DLM, divisor >> 8);

	/* Set data format */
	console_write(halconsole_common.base, LCR, 0x03);

	/* Set DTR and RTS */
	console_write(halconsole_common.base, MCR, 0x03);

	/* Enable and configure FIFOs */
	console_write(halconsole_common.base, FCR, 0xa7);

	/* Disable interrupts */
	console_write(halconsole_common.base, IER, 0x00);
}
