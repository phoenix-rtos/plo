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

#include <hal/hal.h>


struct {
	volatile u32 *uart;
} halconsole_common;


enum {
	cr = 0, mr, ier, idr, imr, isr, baudgen, rxtout, rxwm, modemcr, modemsr, sr, fifo,
	baud_rate_divider_reg0, flow_delay_reg0, tx_fifo_trigger_level0,
};


void hal_consolePrint(const char *s)
{
	for (; *s; s++) {
		/* Wait until TX fifo is empty */
		while (!(*(halconsole_common.uart + sr) & (1 << 3)))
			;

		*(halconsole_common.uart + fifo) = *s;
	}
}

static void hal_initClock(void)
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
	_zynq_setAmbaClk(amba_uart1_clk, clk_enable);
}


static void hal_setPin(u32 pin)
{
	ctl_mio_t ctl;

	/* Set default properties for UART's pins */
	ctl.pin = pin;
	ctl.l0 = ctl.l1 = ctl.l2 = 0;
	ctl.l3 = 0x7;
	ctl.speed = 0;
	ctl.ioType = 1;
	ctl.pullup = 0;
	ctl.disableRcvr = 0;

	switch (pin)
	{
		/* Uart Rx */
		case mio_pin_10:
		case mio_pin_49:
			ctl.triEnable = 1;
			break;

		/* Uart Tx */
		case mio_pin_11:
		case mio_pin_48:
			ctl.triEnable = 0;
			break;

		default:
			return;
	}

	_zynq_setMIO(&ctl);
}


void hal_consoleInit(void)
{
	hal_initClock();
	hal_setPin(UART1_RX);
	hal_setPin(UART1_TX);

	halconsole_common.uart = UART1_BASE_ADDR;

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
