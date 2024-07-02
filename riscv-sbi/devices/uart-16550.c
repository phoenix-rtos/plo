// implement uart-16550 driver, only for writing and no interrupts

/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include "fdt.h"
#include "hart.h"
#include "types.h"

#include "devices/console.h"


typedef struct {
	unsigned int speed;
	unsigned int divisor;
	unsigned char mode;
} baud_t;


/* UART 16550 registers */
enum {
	rbr = 0, /* Receiver Buffer Register */
	thr = 0, /* Transmitter Holding Register */
	dll = 0, /* Divisor Latch LSB */
	ier = 1, /* Interrupt Enable Register */
	dlm = 1, /* Divisor Latch MSB */
	iir = 2, /* Interrupt Identification Register */
	fcr = 2, /* FIFO Control Register */
	lcr = 3, /* Line Control Register */
	mcr = 4, /* Modem Control Register */
	lsr = 5, /* Line Status Register */
	msr = 6, /* Modem Status Register */
	spr = 7, /* Scratch Pad Register */
};


static struct {
	volatile u8 *base;
} uart16550_common;


static void setReg(u8 reg, u8 val)
{
	*(uart16550_common.base + reg) = val;
	RISCV_FENCE(w, o);
}


static u8 readReg(u8 reg)
{
	u8 val = *(uart16550_common.base + reg);
	RISCV_FENCE(i, r);
	return val;
}


static void uart16550_putc(char c)
{
	/* Wait until THR is empty */
	while ((readReg(lsr) & 0x40) == 0) { }
	setReg(thr, c);
}


static int uart16550_getc(void)
{
	if ((readReg(lsr) & 0x01) != 0) {
		return readReg(rbr);
	}

	return -1;
}


static int uart16550_init(const char *compatible)
{
	u16 bdiv;
	uart_info_t info;
	if (fdt_getUartInfo(&info, compatible) < 0) {
		return -1;
	}
	uart16550_common.base = (vu8 *)info.reg.base;
	bdiv = (info.freq + 8 * info.baud) / (16 * info.baud);

	/* Disable all interrupts */
	setReg(ier, 0x00);

	/* Set speed */
	setReg(lcr, 0x80);
	setReg(dll, bdiv & 0xff);
	setReg(dlm, (bdiv >> 8) & 0xff);

	/* 8 bits, no parity, one stop bit */
	setReg(lcr, 0x03);

	/* Enable FIFO */
	setReg(fcr, 0x01);

	/* No modem control DTR RTS */
	setReg(mcr, 0x00);

	/* Clear line status */
	readReg(lsr);
	/* Read receive buffer */
	readReg(rbr);
	/* Set scratchpad */
	setReg(spr, 0x00);

	return 0;
}


static const uart_driver_t uart_16550[] __attribute__((section("uart_drivers"), used)) = {
	{ .compatible = "ns16550a", .init = uart16550_init, .putc = uart16550_putc, .getc = uart16550_getc },
	{ .compatible = "ns16550", .init = uart16550_init, .putc = uart16550_putc, .getc = uart16550_getc },
};
