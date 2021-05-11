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
#include "peripherals.h"


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

	halconsole_common.uart = UART10_BASE;

	_imxrt_setDevClock(UART10_CLK, 0, 0, 0, 0, 1);

	/* tx */
	_imxrt_setIOmux(pctl_mux_gpio_ad_24, 0, 0);
	_imxrt_setIOpad(pctl_pad_gpio_ad_24, 0, 0, 0, 0, 0, 0);

	/* rx */
	_imxrt_setIOmux(pctl_mux_gpio_ad_25, 0, 0);
	_imxrt_setIOpad(pctl_pad_gpio_ad_25, 0, 0, 1, 1, 0, 0);

	_imxrt_setIOisel(pctl_isel_lpuart1_rxd, 0);
	_imxrt_setIOisel(pctl_isel_lpuart1_txd, 0);

	/* Reset all internal logic and registers, except the Global Register */
	*(halconsole_common.uart + uart_global) |= 1 << 1;
	imxrt_dataBarrier();
	*(halconsole_common.uart + uart_global) &= ~(1 << 1);
	imxrt_dataBarrier();

	/* Set 115200 baudrate */
	t = *(halconsole_common.uart + uart_baud) & ~((0x1f << 24) | (1 << 17) | 0xfff);
	*(halconsole_common.uart + uart_baud) = t | 50462772;

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
