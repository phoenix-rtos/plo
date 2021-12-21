/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Console
 *
 * Copyright 2021 Phoenix Systems
 * Authors: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "stm32l4.h"


static struct {
	volatile u32 *base;
	unsigned cpufreq;
} halconsole_common;


enum { cr1 = 0, cr2, cr3, brr, gtpr, rtor, rqr, isr, icr, rdr, tdr };


void hal_consolePrint(const char *s)
{
	while (*s) {
		if (~(*(halconsole_common.base + isr)) & 0x80)
			continue;

		*(halconsole_common.base + tdr) = *(s++);
	}

	while (~(*(halconsole_common.base + isr)) & 0x80)
		;

	return;
}


void console_init(void)
{
	struct {
		void *base;
		u8 uart;
	} uarts[] = {
		{ (void *)UART1_BASE, UART1_CLK },
		{ (void *)UART2_BASE, UART2_CLK },
		{ (void *)UART3_BASE, UART3_CLK },
		{ (void *)UART4_BASE, UART4_CLK },
		{ (void *)UART5_BASE, UART5_CLK }
	};

	const int uart = 1, port = pctl_gpiod, txpin = 5, rxpin = 6, af = 7;

	_stm32_rccSetDevClock(port, 1);

	halconsole_common.base = uarts[uart].base;

	/* Init tx pin - output, push-pull, high speed, no pull-up */
	_stm32_gpioConfig(port, txpin, 2, af, 0, 2, 0);

	/* Init rxd pin - input, push-pull, high speed, no pull-up */
	_stm32_gpioConfig(port, rxpin, 2, af, 0, 2, 0);

	/* Enable uart clock */
	_stm32_rccSetDevClock(uarts[uart].uart, 1);

	halconsole_common.cpufreq = _stm32_rccGetCPUClock();

	/* Set up UART to 9600,8,n,1 16-bit oversampling */
	*(halconsole_common.base + cr1) &= ~1UL; /* disable USART */
	hal_cpuDataMemoryBarrier();
	*(halconsole_common.base + cr1) = 0xa;
	*(halconsole_common.base + cr2) = 0;
	*(halconsole_common.base + cr3) = 0;
	*(halconsole_common.base + brr) = halconsole_common.cpufreq / 115200; /* 115200 baud rate */
	hal_cpuDataMemoryBarrier();
	*(halconsole_common.base + cr1) |= 1;
	hal_cpuDataMemoryBarrier();
}
