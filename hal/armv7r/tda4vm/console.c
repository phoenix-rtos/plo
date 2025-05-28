/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console (TDA4VM, 16550-compatible)
 *
 * Copyright 2021, 2025 Phoenix Systems
 * Authors: Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#include <board_config.h>
#include "tda4vm.h"


#if UART_CONSOLE_PLO == 0
#define UART_RX       UART0_RX
#define UART_TX       UART0_TX
#define UART_BAUDRATE UART0_BAUDRATE
#else
#error "TODO: support other UARTs for console"
#endif


struct {
	volatile u32 *uart;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


/* UART registers */
enum {
	rbr = 0, /* Receiver Buffer Register */
	thr = 0, /* Transmitter Holding Register */
	dll = 0, /* Divisor Latch LSB */
	ier = 1, /* Interrupt Enable Register */
	dlm = 1, /* Divisor Latch MSB */
	iir = 2, /* Interrupt Identification Register */
	fcr = 2, /* FIFO Control Register */
	efr = 2, /* Enhanced feature register */
	lcr = 3, /* Line Control Register */
	mcr = 4, /* Modem Control Register */
	lsr = 5, /* Line Status Register */
	msr = 6, /* Modem Status Register */
	spr = 7, /* Scratch Pad Register */
	/* Below are extensions */
	mdr1 = 8, /* Mode definition register 1 */
	mdr2 = 9, /* Mode definition register 2 */
};


static unsigned char reg_read(unsigned int reg)
{
	return *(halconsole_common.uart + reg);
}


static void reg_write(unsigned int reg, unsigned char val)
{
	*(halconsole_common.uart + reg) = val;
}


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


void hal_consolePrint(const char *s)
{
	const char *ptr;

	for (ptr = s; *ptr != '\0'; ++ptr) {
		/* Wait until TX fifo is empty */
		while ((reg_read(lsr) & 0x20) == 0) {
		}
		reg_write(thr, *ptr);
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


static u32 console_initClock(const tda4vm_uart_info_t *info)
{
	if (info->clksel >= 0) {
		tda4vm_setClksel(info->clksel, info->clksel_val);
	}

	if (info->clkdiv >= 0) {
		tda4vm_setClkdiv(info->clkdiv, info->divisor);
	}

	return (u32)tda4vm_getFrequency(info->pll, info->hsdiv) / info->divisor;
}


static unsigned console_calcDivisor(u32 baseClock)
{
	/* Assume we are in UART x16 mode */
	const unsigned baud_16 = (16 * UART_BAUDRATE);
	unsigned divisor, remainder;
	divisor = baseClock / baud_16;
	remainder = baseClock % baud_16;
	divisor += (remainder >= (baud_16 / 2)) ? 1 : 0;

	/* On this platform DLH register only holds 6 bits - so limit to 14 bits */
	return (divisor >= (1 << 14)) ? ((1 << 14) - 1) : divisor;
}


static void console_setPin(const tda4vm_uart_info_t *info, u32 pin)
{
	int muxSetting, isTx;
	tda4vm_pinConfig_t cfg;
	for (unsigned i = 0; i < MAX_PINS_PER_UART; i++) {
		if (info->pins[i].pin < 0) {
			return;
		}

		if (info->pins[i].pin == pin) {
			muxSetting = info->pins[i].muxSetting;
			isTx = info->pins[i].isTx;
			cfg.debounce_idx = 0;
			cfg.mux = muxSetting & 0xff;
			if (isTx) {
				cfg.flags = TDA4VM_GPIO_PULL_DISABLE;
			}
			else {
				cfg.flags = TDA4VM_GPIO_RX_EN | TDA4VM_GPIO_PULL_DISABLE;
			}

			tda4vm_setPinConfig(pin, &cfg);
			return;
		}
	}
}


void console_init(void)
{
	unsigned int n = UART_CONSOLE_PLO;
	u32 baseClock, divisor;
	const tda4vm_uart_info_t *info;
	info = tda4vm_getUartInfo(n);
	if (info == NULL) {
		return;
	}

	halconsole_common.uart = info->base;
	while ((reg_read(lsr) & 0x40) == 0) {
		/* Wait until all data is shifted out */
	}

	baseClock = console_initClock(info);
	console_setPin(info, UART_RX);
	console_setPin(info, UART_TX);


	/* Put into UART x16 mode */
	reg_write(mdr1, 0x0);

	/* Enable enhanced functions */
	reg_write(lcr, 0xbf);
	reg_write(efr, 1 << 4);
	reg_write(lcr, 0x0);

	/* Set DTR and RTS */
	reg_write(mcr, 0x03);

	/* Enable and configure FIFOs */
	reg_write(fcr, 0xa7);

	/* Set speed */
	divisor = console_calcDivisor(baseClock);
	reg_write(lcr, 0x80);
	reg_write(dll, divisor & 0xff);
	reg_write(dlm, (divisor >> 8) & 0xff);

	/* Set data format */
	reg_write(lcr, 0x03);

	/* Disable interrupts */
	reg_write(ier, 0x00);
}
