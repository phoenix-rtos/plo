/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * UART 16550 (TDA4VM)
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include <board_config.h>

#include "uart-16550.h"


/* UART extension registers */
enum {
	efr = 2,  /* Enhanced feature register */
	mdr1 = 8, /* Mode definition register 1 */
	mdr2 = 9, /* Mode definition register 2 */
};

typedef struct {
	volatile u32 *base;
	unsigned int irq;
	unsigned int clk;
} uarthw_context_t;

static uarthw_context_t uarts[UART_MAX_CNT];


static const struct {
	int tx;
	int rx;
} uarthw_pins[] = {
	{ UART0_TX, UART0_RX },
};


unsigned char uarthw_read(void *hwctx, unsigned int reg)
{
	return *(((uarthw_context_t *)hwctx)->base + reg);
}


void uarthw_write(void *hwctx, unsigned int reg, unsigned char val)
{
	*(((uarthw_context_t *)hwctx)->base + reg) = val;
}


unsigned int uarthw_irq(void *hwctx)
{
	return ((uarthw_context_t *)hwctx)->irq;
}


unsigned int uarthw_getDivisor(void *hwctx, baud_t *baud)
{
	/* Assume we are in UART x16 mode */
	const u32 baud_16 = (16 * baud->speed);
	u32 baseClock = ((uarthw_context_t *)hwctx)->clk;
	u32 divisor, remainder;
	divisor = baseClock / baud_16;
	remainder = baseClock % baud_16;
	divisor += (remainder >= (baud_16 / 2)) ? 1 : 0;

	/* On this platform DLH register only holds 6 bits - so limit to 14 bits */
	return (divisor >= (1 << 14)) ? ((1 << 14) - 1) : divisor;
}


static void uarthw_setPin(const tda4vm_uart_info_t *info, u32 pin)
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


int uarthw_init(unsigned int n, void *hwctx, unsigned int *baud)
{
	const tda4vm_uart_info_t *info = tda4vm_getUartInfo(n);
	if (info == NULL) {
		return -EINVAL;
	}

	if (n >= (sizeof(uarthw_pins) / sizeof(uarthw_pins[0]))) {
		return -EINVAL;
	}

	if (info->irq < 0) {
		return -EINVAL;
	}

	if (info->clksel >= 0) {
		tda4vm_setClksel(info->clksel, info->clksel_val);
	}

	if (info->clkdiv >= 0) {
		tda4vm_setClkdiv(info->clkdiv, info->divisor);
	}

	uarts[n].base = info->base;
	uarts[n].irq = info->irq;
	uarts[n].clk = (u32)tda4vm_getFrequency(info->pll, info->hsdiv) / info->divisor;

	/* Set up pins */
	uarthw_setPin(info, uarthw_pins[n].tx);
	uarthw_setPin(info, uarthw_pins[n].rx);

	/* Set UART hardware context */
	*(uarthw_context_t *)hwctx = uarts[n];

	/* Put into UART x16 mode */
	uarthw_write(hwctx, mdr1, 0x0);

	/* Enable enhanced functions */
	uarthw_write(hwctx, lcr, 0xbf);
	uarthw_write(hwctx, efr, 1 << 4);
	uarthw_write(hwctx, lcr, 0x0);

	/* Set preferred baudrate */
	if (baud != NULL) {
		*baud = bps_115200;
	}

	return EOK;
}
