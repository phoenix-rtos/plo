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

#include <board_config.h>

#if UART_CONSOLE_PLO == 0
#define UART_RX        UART0_RX
#define UART_TX        UART0_TX
#define UART_BASE_ADDR UART0_BASE_ADDR
#define UART_PLL       UART0_PLL
#define UART_RESET     UART0_RESET
#else
#define UART_RX        UART1_RX
#define UART_TX        UART1_TX
#define UART_BASE_ADDR UART1_BASE_ADDR
#define UART_PLL       UART1_PLL
#define UART_RESET     UART1_RESET
#endif

struct {
	volatile u32 *uart;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


enum {
	/* clang-format off */
	cr = 0, mr, ier, idr, imr, isr, baudgen, rxtout, rxwm, modemcr, modemsr, sr, fifo,
	baud_rate_divider_reg0, flow_delay_reg0, tx_fifo_trigger_level0,
	/* clang-format on */
};


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


void hal_consolePrint(const char *s)
{
	const char *ptr;

	for (ptr = s; *ptr != '\0'; ++ptr) {
		/* Wait until TX fifo is empty */
		while ((*(halconsole_common.uart + sr) & (1 << 3)) == 0) {
		}
		*(halconsole_common.uart + fifo) = *ptr;
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


static void console_initClock(void)
{
	ctl_clock_t ctl;

	/* Set IO PLL as source clock and set divider:
	 * IO_PLL / 0x14 :  99.99 MHz / 20 = 50 MHz     */
	ctl.dev = UART_PLL;
	ctl.active = 1;
	ctl.src = 0;
	ctl.div0 = 20;
	ctl.div1 = 0;

	_zynqmp_setCtlClock(&ctl);
	_zynqmp_devReset(UART_RESET, 0);
}


static void console_setPin(u32 pin)
{
	ctl_mio_t ctl;

	/* Set default properties for UART's pins */
	ctl.pin = pin;
	ctl.l0 = ctl.l1 = ctl.l2 = 0;
	ctl.l3 = 0x6;
	ctl.config = MIO_SLOW_nFAST | MIO_PULL_UP_nDOWN | MIO_PULL_ENABLE;

	switch (pin) {
		case UART0_RX: /* Fall-through */
		case UART1_RX:
			ctl.config |= MIO_TRI_ENABLE;
			break;

		case UART0_TX: /* Fall-through */
		case UART1_TX:
			/* Do nothing */
			break;

		default:
			return;
	}

	_zynqmp_setMIO(&ctl);
}


void console_init(void)
{
	console_initClock();

#if (UART_CONSOLE_ROUTED_VIA_PL != 1)
	console_setPin(UART_RX);
	console_setPin(UART_TX);
#else
	(void)console_setPin;
#endif

	halconsole_common.uart = UART_BASE_ADDR;

	*(halconsole_common.uart + idr) = 0xfff;
	/* Uart Mode Register
	 * normal mode, 1 stop bit, no parity, 8 bits, uart_ref_clk as source clock
	 * PAR = 0x4 */
	*(halconsole_common.uart + mr) = (*(halconsole_common.uart + mr) & ~0x000003ff) | 0x00000020;

	/* Disable TX and RX */
	*(halconsole_common.uart + cr) = (*(halconsole_common.uart + cr) & ~0x000001ff) | 0x00000028;

	/* Assumptions:
	 * - baudarate : 115200
	 * - ref_clk   : 50 MHz
	 * - baud_rate = ref_clk / (bgen * (bdiv + 1)) */
	*(halconsole_common.uart + baudgen) = 62;
	*(halconsole_common.uart + baud_rate_divider_reg0) = 6;

	/* Uart Control Register
	 * TXEN = 0x1; RXEN = 0x1; TXRES = 0x1; RXRES = 0x1 */
	*(halconsole_common.uart + cr) = (*(halconsole_common.uart + cr) & ~0x000001ff) | 0x00000017;
}
