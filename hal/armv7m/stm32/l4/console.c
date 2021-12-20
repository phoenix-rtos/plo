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

	while (~(*(halconsole_common.base + isr)) & 0x80);

	return;
}


void console_init(void)
{
	struct {
		void *base;
		u8 uart;
	} uarts[] = {
		{ (void *)0x40013800, pctl_usart1 }, /* USART1 */
		{ (void *)0x40004400, pctl_usart2 }, /* USART2 */
		{ (void *)0x40004800, pctl_usart3 }, /* USART3 */
		{ (void *)0x40004c00, pctl_uart4 }, /* UART4 */
		{ (void *)0x40005000, pctl_uart5 }  /* UART5 */
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
	_stm32_dataBarrier();
	*(halconsole_common.base + cr1) = 0xa;
	*(halconsole_common.base + cr2) = 0;
	*(halconsole_common.base + cr3) = 0;
	*(halconsole_common.base + brr) = halconsole_common.cpufreq / 115200; /* 115200 baud rate */
	_stm32_dataBarrier();
	*(halconsole_common.base + cr1) |= 1;
	_stm32_dataBarrier();
}
