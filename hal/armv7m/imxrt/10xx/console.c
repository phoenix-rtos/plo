/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2021-2023 Phoenix Systems
 * Authors: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <board_config.h>

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

#if CONSOLE_BAUD(UART_CONSOLE_PLO)
#define CONSOLE_BAUDRATE CONSOLE_BAUD(UART_CONSOLE_PLO)
#else
#define CONSOLE_BAUDRATE (UART_BAUDRATE)
#endif


struct {
	volatile u32 *uart;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


/* clang-format off */

enum { uart_verid = 0, uart_param, uart_global, uart_pincfg, uart_baud, uart_stat, uart_ctrl,
	uart_data, uart_match, uart_modir, uart_fifo, uart_water };

/* clang-format on */


static int console_muxVal(int mux)
{
	switch (mux) {
		case pctl_mux_gpio_b1_12:
		case pctl_mux_gpio_b1_13:
			return 1;

		case pctl_mux_gpio_b0_08:
		case pctl_mux_gpio_b0_09:
			return 3;

		case pctl_mux_gpio_sd_b1_00:
		case pctl_mux_gpio_sd_b1_01:
			return 4;

		default: break;
	}

	return 2;
}


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


void hal_consolePrint(const char *s)
{
	const char *ptr;

	for (ptr = s; *ptr != '\0'; ++ptr) {
		while ((*(halconsole_common.uart + uart_stat) & (1uL << 23)) == 0uL) {
		}
		*(halconsole_common.uart + uart_data) = *ptr;
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


void console_init(void)
{
	u32 t;

	halconsole_common.uart = CONSOLE_BASE(UART_CONSOLE_PLO);

	_imxrt_ccmControlGate(CONSOLE_CLK(UART_CONSOLE_PLO), clk_state_run_wait);

	/* tx */
	_imxrt_setIOmux(CONSOLE_MUX(UART_CONSOLE_PLO, TX_PIN), 0,
		console_muxVal(CONSOLE_MUX(UART_CONSOLE_PLO, TX_PIN)));
	_imxrt_setIOpad(CONSOLE_PAD(UART_CONSOLE_PLO, TX_PIN), 0, 0, 0, 1, 0, 2, 6, 0);

	/* rx */
	_imxrt_setIOmux(CONSOLE_MUX(UART_CONSOLE_PLO, RX_PIN), 0,
		console_muxVal(CONSOLE_MUX(UART_CONSOLE_PLO, RX_PIN)));
	_imxrt_setIOpad(CONSOLE_PAD(UART_CONSOLE_PLO, RX_PIN), 0, 0, 0, 1, 0, 2, 6, 0);

#if (UART_CONSOLE_PLO >= 2) && (UART_CONSOLE_PLO <= 8)
	_imxrt_setIOisel(CONSOLE_ISEL(UART_CONSOLE_PLO, rx), 1);
	_imxrt_setIOisel(CONSOLE_ISEL(UART_CONSOLE_PLO, tx), 1);
#endif

	_imxrt_ccmSetMux(clk_mux_uart, 0);
	_imxrt_ccmSetDiv(clk_div_uart, 0);

	/* Reset all internal logic and registers, except the Global Register */
	*(halconsole_common.uart + uart_global) |= 1 << 1;
	hal_cpuDataMemoryBarrier();
	*(halconsole_common.uart + uart_global) &= ~(1 << 1);
	hal_cpuDataMemoryBarrier();

	/* Set baud rate */
	t = *(halconsole_common.uart + uart_baud) & ~((0x1f << 24) | (1 << 17) | 0x1fff);

	/* For baud rate calculation, default UART_CLK=80MHz was assumed */
	switch (CONSOLE_BAUDRATE) {
		case 9600: t |= 0x0c000281; break;
		case 19200: t |= 0x080001cf; break;
		case 38400: t |= 0x03020209; break;
		case 57600: t |= 0x0302015b; break;
		case 115200: t |= 0x0402008b; break;
		case 230400: t |= 0x1c00000c; break;
		case 460800: t |= 0x1c000006; break;
		/* As fallback use default 115200 */
		default: t |= 0x0402008b; break;
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
