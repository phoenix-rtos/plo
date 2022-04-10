/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * nRF9160 basic peripherals control functions
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include "nrf91.h"


static struct {
	volatile u32 *power;
	volatile u32 *clock;
	volatile u32 *gpio;
} nrf91_common;


/* clang-format off */
enum { power_tasks_constlat = 30, power_tasks_lowpwr, power_inten = 192, power_intenset, power_intenclr, power_status = 272};


enum { clock_tasks_hfclkstart, clock_inten = 192, clock_intenset, clock_intenclr, clock_hfclkrun = 258, clock_hfclkstat };


enum { gpio_out = 1, gpio_outset, gpio_outclr, gpio_in, gpio_dir, gpio_dirsetout, gpio_dirsetin, gpio_cnf = 128 };
/* clang-format on */


/* GPIO */


int _nrf91_gpioConfig(u8 pin, u8 dir, u8 pull)
{
	if (pin > 31) {
		return -EINVAL;
	}

	if (dir == gpio_output) {
		*(nrf91_common.gpio + gpio_dirsetout) = (1u << pin);
	}
	else if (dir == gpio_input) {
		*(nrf91_common.gpio + gpio_dirsetin) = (1u << pin);
		/* connect gpio_input buffer */
		*(nrf91_common.gpio + gpio_cnf + pin) &= ~0x2u;
	}
	else {
		return -EINVAL;
	}

	if (pull) {
		*(nrf91_common.gpio + gpio_cnf + pin) = (pull << 2);
	}

	hal_cpuDataMemoryBarrier();

	return EOK;
}


int _nrf91_gpioSet(u8 pin, u8 val)
{
	if (pin > 31)
		return -EINVAL;

	if (val == gpio_high) {
		*(nrf91_common.gpio + gpio_outset) = (1u << pin);
	}
	else if (val == gpio_low) {
		*(nrf91_common.gpio + gpio_outclr) = (1u << pin);
	}
	else {
		return -EINVAL;
	}

	hal_cpuDataMemoryBarrier();

	return EOK;
}


void _nrf91_init(void)
{
	nrf91_common.power = (void *)0x50005000u;
	nrf91_common.clock = (void *)0x50005000u;
	nrf91_common.gpio = (void *)0x50842500u;

	/* Enable gpio_low power mode */
	*(nrf91_common.power + power_tasks_lowpwr) = 1u;

	/* Disable all power interrupts */
	*(nrf91_common.power + power_intenclr) = 0x64u;

	/* Disable all clock interrupts */
	*(nrf91_common.power + power_intenclr) = 0x3u;

	hal_cpuDataMemoryBarrier();

	*(nrf91_common.clock + clock_tasks_hfclkstart) = 1u;
	/* Wait until HXFO start and clear event flag */
	while (*(nrf91_common.clock + clock_hfclkrun) != 1u) {
		;
	}
	*(nrf91_common.clock + clock_hfclkrun) = 0u;

	hal_cpuDataMemoryBarrier();
}
