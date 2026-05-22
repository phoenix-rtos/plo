/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Authors: Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#define UART_BASE_ADDR 0x09000000

struct {
	volatile u32 *uart;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;

enum {
	dr = 0x00,
	fr = 0x06,
	lcr_h = 0x0b,
	cr = 0x0c,
	imsc = 0x0e,
	icr = 0x11,
};

#define PL011_FR_TXFF (1 << 5) /* Transmit FIFO Full */

void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}

void hal_consolePrint(const char *s)
{
	const char *ptr;

	for (ptr = s; *ptr != '\0'; ++ptr) {
		/* Wait until TX fifo is empty */
		while ((*(halconsole_common.uart + fr) & PL011_FR_TXFF) != 0) {
		}
		*(halconsole_common.uart + dr) = *ptr;
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}

void console_init(void)
{
	halconsole_common.uart = (volatile u32 *)UART_BASE_ADDR;

	*(halconsole_common.uart + cr) = 0x0;

	*(halconsole_common.uart + icr) = 0x7ff;

	*(halconsole_common.uart + imsc) = 0x0;

	/* WLEN = 8 bits (0x60), Enable FIFOs (0x10) -> 0x70 */
	*(halconsole_common.uart + lcr_h) = 0x70;

	/* CR Bit 0 = UARTEN, Bit 8 = TXE, Bit 9 = RXE -> 0x301 */
	*(halconsole_common.uart + cr) = (1 << 0) | (1 << 8) | (1 << 9);
}
