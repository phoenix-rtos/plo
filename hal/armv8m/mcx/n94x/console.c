/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Console
 *
 * Copyright 2021-2023, 2024 Phoenix Systems
 * Authors: Hubert Buczynski, Gerard Swiderski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "n94x.h"
#include <board_config.h>


enum { uart_verid = 0, uart_param, uart_global, uart_pincfg, uart_baud,
	uart_stat, uart_ctrl, uart_data, uart_match, uart_modir, uart_fifo,
	uart_water, uart_dataro, uart_mcr = 16, uart_msr, uart_reir, uart_teir,
	uart_hdcr, uart_tocr = 22, uart_tosr, uart_timeoutn, uart_tcbrn = 128,
	uart_tdbrn = 256 };


static struct {
	volatile u32 *base;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


void hal_consolePrint(const char *s)
{
	const char *ptr;

	for (ptr = s; *ptr != '\0'; ++ptr) {
		while ((*(halconsole_common.base + uart_stat) & (1uL << 23)) == 0uL) {
		}
		*(halconsole_common.base + uart_data) = *ptr;
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


void console_init(void)
{
	u32 t;
	static struct {
		volatile u32 *base;
	} info[10] = {
		{ .base = FC0_BASE },
		{ .base = FC1_BASE },
		{ .base = FC2_BASE },
		{ .base = FC3_BASE },
		{ .base = FC4_BASE },
		{ .base = FC5_BASE },
		{ .base = FC6_BASE },
		{ .base = FC7_BASE },
		{ .base = FC8_BASE },
		{ .base = FC9_BASE },
	};

	halconsole_common.base = info[UART_CONSOLE].base;

	/* Reset all internal logic and registers, except the Global Register */
	*(halconsole_common.base + uart_global) |= 1 << 1;
	hal_cpuDataMemoryBarrier();
	*(halconsole_common.base + uart_global) &= ~(1 << 1);
	hal_cpuDataMemoryBarrier();

	/* Set baud rate */
	t = *(halconsole_common.base + uart_baud) & ~((0xf << 24) | (1 << 17) | 0x1fff);

	/* For baud rate calculation, default UART_CLK=12MHz assumed */
	switch (UART_BAUDRATE) {
		case 9600: t |= 0x03020271; break;
		case 19200: t |= 0x03020138; break;
		case 38400: t |= 0x0302009c; break;
		case 57600: t |= 0x03020068; break;
		case 230400: t |= 0x0302001a; break;
		case 460800: t |= 0x0302000d; break;
		default: t |= 0x03020034; break; /* 115200 */
	}
	*(halconsole_common.base + uart_baud) = t;

	/* Set 8 bit and no parity mode */
	*(halconsole_common.base + uart_ctrl) &= ~0x117;

	/* One stop bit */
	*(halconsole_common.base + uart_baud) &= ~(1 << 13);

	*(halconsole_common.base + uart_water) = 0;

	/* Enable FIFO */
	*(halconsole_common.base + uart_fifo) |= (1 << 7) | (1 << 3);
	*(halconsole_common.base + uart_fifo) |= 0x3 << 14;

	/* Clear all status flags */
	*(halconsole_common.base + uart_stat) |= 0xc01fc000;

	/* Enable TX and RX */
	*(halconsole_common.base + uart_ctrl) |= (1 << 19) | (1 << 18);
}
