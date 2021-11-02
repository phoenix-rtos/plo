/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2021 Phoenix Systems
 * Authors: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#define CONCAT(a, b)         a##b
#define CONCAT2(a, b)        CONCAT(a, b)
#define UART_PIN(n, pin)     CONCAT(UART, n##_##pin)
#define CONSOLE_BASE(n)      (UART_PIN(n, BASE))
#define CONSOLE_CLK(n)       (UART_PIN(n, CLK))
#define CONSOLE_MUX(n, pin)  (CONCAT2(pctl_mux_gpio_, UART_PIN(n, pin)))
#define CONSOLE_PAD(n, pin)  (CONCAT2(pctl_pad_gpio_, UART_PIN(n, pin)))
#define CONSOLE_ISEL(n, pin) (CONCAT2(pctl_isel_lpuart, CONCAT(n, _##pin)))
#define CONSOLE_BAUD(n)      (UART_PIN(n, BAUDRATE))

#if CONSOLE_BAUD(UART_CONSOLE)
#define CONSOLE_BAUDRATE CONSOLE_BAUD(UART_CONSOLE)
#else
#define CONSOLE_BAUDRATE (UART_BAUDRATE)
#endif


struct {
	volatile u32 *uart;
} halconsole_common;


enum { uart_verid = 0, uart_param, uart_global, uart_pincfg, uart_baud, uart_stat, uart_ctrl,
	   uart_data, uart_match, uart_modir, uart_fifo, uart_water };


void hal_consolePrint(const char *s)
{
	while (*s) {
		while (!(*(halconsole_common.uart + uart_stat) & (1 << 23)))
			;
		*(halconsole_common.uart + uart_data) = *(s++);
	}
}


void console_init(void)
{
	u32 t;

	halconsole_common.uart = CONSOLE_BASE(UART_CONSOLE);

	_imxrt_setDevClock(CONSOLE_CLK(UART_CONSOLE), 0, 0, 0, 0, 1);

	/* tx */
	_imxrt_setIOmux(CONSOLE_MUX(UART_CONSOLE, TX_PIN), 0, 0);
	_imxrt_setIOpad(CONSOLE_PAD(UART_CONSOLE, TX_PIN), 0, 0, 0, 0, 0, 0);

	/* rx */
	_imxrt_setIOmux(CONSOLE_MUX(UART_CONSOLE, RX_PIN), 0, 0);
	_imxrt_setIOpad(CONSOLE_PAD(UART_CONSOLE, RX_PIN), 0, 0, 1, 1, 0, 0);

#if (UART_CONSOLE == 1) || (UART_CONSOLE == 12)
	_imxrt_setIOisel(CONSOLE_ISEL(UART_CONSOLE, txd), 0);
	_imxrt_setIOisel(CONSOLE_ISEL(UART_CONSOLE, rxd), 0);
#endif

	/* Reset all internal logic and registers, except the Global Register */
	*(halconsole_common.uart + uart_global) |= 1 << 1;
	hal_cpuDataMemoryBarrier();
	*(halconsole_common.uart + uart_global) &= ~(1 << 1);
	hal_cpuDataMemoryBarrier();

	/* Set baud rate */
	t = *(halconsole_common.uart + uart_baud) & ~((0x1f << 24) | (1 << 17) | 0x1fff);

	/* For baud rate calculation, default UART_CLK=24MHz assumed */
	switch (CONSOLE_BAUDRATE) {
		case 9600: t |= 0x03020271; break;
		case 19200: t |= 0x03020138; break;
		case 38400: t |= 0x0302009c; break;
		case 57600: t |= 0x03020068; break;
		case 115200: t |= 0x03020034; break;
		case 230400: t |= 0x0302001a; break;
		case 460800: t |= 0x0302000d; break;
		/* As fallback use default 115200 */
		default: t |= 0x03020034; break;
	}
	*(halconsole_common.uart + uart_baud) = t;

	/* Set 8 bit and no parity mode */
	*(halconsole_common.uart + uart_ctrl) &= ~0x117;

	/* One stop bit */
	*(halconsole_common.uart + uart_baud) &= ~(1 << 13);

	*(halconsole_common.uart + uart_water) = 0;

	/* Enable FIFO */
	*(halconsole_common.uart + uart_fifo) |= (1 << 7) | (1 << 3);
	*(halconsole_common.uart + uart_fifo) |= 0x3 << 14;

	/* Clear all status flags */
	*(halconsole_common.uart + uart_stat) |= 0xc01fc000;

	/* Enable TX and RX */
	*(halconsole_common.uart + uart_ctrl) |= (1 << 19) | (1 << 18);
}
