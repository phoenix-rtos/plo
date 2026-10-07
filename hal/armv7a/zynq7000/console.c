/*
 * Phoenix-RTOS
 *
 * Operating system loader
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

#include <hal/hal.h>

#include <board_config.h>

#if UART_CONSOLE_PLO == 0
#define UART_RX        UART0_RX
#define UART_TX        UART0_TX
#define UART_BASE_ADDR UART0_BASE_ADDR
#define UART_CLK       UART0_CLK
#else
#define UART_RX        UART1_RX
#define UART_TX        UART1_TX
#define UART_BASE_ADDR UART1_BASE_ADDR
#define UART_CLK       UART1_CLK
#endif

struct {
	volatile u32 *uart;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


enum {
	cr = 0, mr, ier, idr, imr, isr, baudgen, rxtout, rxwm, modemcr, modemsr, sr, fifo,
	baud_rate_divider_reg0, flow_delay_reg0, tx_fifo_trigger_level0,
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
	 * IO_PLL / 0x14 :  1000 MHz / 20 = 50 MHz     */
	ctl.dev = ctrl_uart_clk;
	ctl.pll.clkact0 = 0x1;
	ctl.pll.clkact1 = 0x1;
	ctl.pll.srcsel = 0;
	ctl.pll.divisor0 = 0x14;

	_zynq_setCtlClock(&ctl);
	_zynq_setAmbaClk(UART_CLK, clk_enable);
}


static void console_setPin(u32 pin)
{
	ctl_mio_t ctl;

	/* Set default properties for UART's pins */
	ctl.pin = pin;
	ctl.l0 = ctl.l1 = ctl.l2 = 0;
	ctl.l3 = 0x7;
	ctl.speed = 0;
	ctl.ioType = 1;
	ctl.pullup = 1;
	ctl.disableRcvr = 1;

	switch (pin) {
		case UART0_RX:
		case UART1_RX:
			ctl.triEnable = 1;
			break;

		case UART0_TX:
		case UART1_TX:
			ctl.triEnable = 0;
			break;

		default:
			return;
	}

	_zynq_setMIO(&ctl);
}


void console_init(void)
{
	console_initClock();
	console_setPin(UART_RX);
	console_setPin(UART_TX);

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
