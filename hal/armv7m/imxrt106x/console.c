/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Console
 *
 * Copyright 2021 Phoenix Systems
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "hal.h"
#include "imxrt.h"


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


void hal_consoleInit(void)
{
	u32 t;

#if UART_CONSOLE == 3
	console_common.uart = (void *)0x4018c000;

	_imxrt_ccmControlGate(pctl_clk_lpuart3, clk_state_run_wait);

	_imxrt_setIOmux(pctl_mux_gpio_emc_13, 0, 2);
	_imxrt_setIOmux(pctl_mux_gpio_emc_14, 0, 2);

	_imxrt_setIOisel(pctl_isel_lpuart3_tx, 1);
	_imxrt_setIOisel(pctl_isel_lpuart3_rx, 1);

	_imxrt_setIOpad(pctl_pad_gpio_emc_13, 0, 0, 0, 1, 0, 2, 6, 0);
	_imxrt_setIOpad(pctl_pad_gpio_emc_14, 0, 0, 0, 1, 0, 2, 6, 0);
#else
	halconsole_common.uart = (void *)0x40184000;

	_imxrt_ccmControlGate(pctl_clk_lpuart1, clk_state_run_wait);

	_imxrt_setIOmux(pctl_mux_gpio_ad_b0_12, 0, 2);
	_imxrt_setIOmux(pctl_mux_gpio_ad_b0_13, 0, 2);
	_imxrt_setIOpad(pctl_pad_gpio_ad_b0_12, 0, 0, 0, 1, 0, 2, 6, 0);
	_imxrt_setIOpad(pctl_pad_gpio_ad_b0_13, 0, 0, 0, 1, 0, 2, 6, 0);
#endif

	_imxrt_ccmSetMux(clk_mux_uart, 0);
	_imxrt_ccmSetDiv(clk_div_uart, 0);

	/* Reset all internal logic and registers, except the Global Register */
	*(halconsole_common.uart + uart_global) |= 1 << 1;
	imxrt_dataBarrier();
	*(halconsole_common.uart + uart_global) &= ~(1 << 1);
	imxrt_dataBarrier();

	/* Set 115200 baudrate */
	t = *(halconsole_common.uart + uart_baud);
	t = (t & ~(0x1f << 24)) | (0x4 << 24);
	*(halconsole_common.uart + uart_baud) = (t & ~0x1fff) | 0x8b;
	*(halconsole_common.uart + uart_baud) &= ~(1 << 29);

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
