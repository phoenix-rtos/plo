/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * HAL console via UART1
 *
 * Copyright 2021 Phoenix Systems
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


enum { urxd = 0, utxd = 16, ucr1 = 32, ucr2, ucr3, ucr4, ufcr, usr1, usr2,
	uesc, utim, ubir, ubmr, ubrc, onems, uts, umcr };


struct {
	volatile u32 *uart;
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
		/* Wait for transmitter readiness */
		while ((*(halconsole_common.uart + usr1) & 0x2000) == 0) {
		}
		*(halconsole_common.uart + utxd) = *ptr;
	}

	while ((*(halconsole_common.uart + usr1) & 0x2000) == 0) {
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


static void console_padsInit(void)
{
	imx6ull_setIOmux(mux_uart1_cts, 0, 0);
	imx6ull_setIOmux(mux_uart1_rts, 0, 0);
	imx6ull_setIOmux(mux_uart1_rx, 0, 0);
	imx6ull_setIOmux(mux_uart1_tx, 0, 0);

	imx6ull_setIOisel(isel_uart1_rts, 3);
	imx6ull_setIOisel(isel_uart1_rx, 3);
}


void console_init(void)
{
	/* UART1 */
	halconsole_common.uart = (void *)(0x02020000);

	imx6ull_setDevClock(clk_uart1, 3);
	console_padsInit();

	/* Controller reset */
	*(halconsole_common.uart + ucr2) &= ~0x1;

	/* Wait until software reset is inactive */
	while (*(halconsole_common.uart + uts) & 0x1)
		;

	/* Enable controller */
	*(halconsole_common.uart + ucr1) = 0x1;
	/* Ignore RTS Pin, 8 -bit word size, enable Tx and TX */
	*(halconsole_common.uart + ucr2) = 0x4026;
	/* Data Terminal Ready Delta Enable, Ring Indicator, Data Carrier Detect,
	 * Data Set Ready */
	*(halconsole_common.uart + ucr3) = 0x704;
	/* 32 characters in the RxFIFO */
	*(halconsole_common.uart + ucr4) = 0x8000;

	/* Transmitter Trigger Level - TxFIFO has 2 or fewer characters,
	 * Divide input clock by 4,
	 * Receiver Trigger Level - RxFIFO has 1 character */
	*(halconsole_common.uart + ufcr) = 0x901;

	/* UART Escape Character */
	*(halconsole_common.uart + uesc) = 0x2b;
	/* UART Escape Timer*/
	*(halconsole_common.uart + utim) = 0x0;

	/* Set baudrate to 115200 */
	*(halconsole_common.uart + ubir) = 0x11ff;
	*(halconsole_common.uart + ubmr) = 0xc34f;
}
