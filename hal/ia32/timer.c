/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * System timer
 *
 * Copyright 2012, 2016, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>


struct {
	/* Number of CPU cycles per 1ms */
	volatile u64 ratio;
} timer_common;


static u64 timer_cycles(void)
{
	u32 lo, hi;

	__asm__ volatile(
		"rdtsc; "
	: "=a" (lo), "=d" (hi));

	return ((u64)hi << 32) | lo;
}


static int timer_isr(unsigned int n, void *arg)
{
	static u64 start;
	u64 t;

	t = timer_cycles();

	if (start)
		timer_common.ratio = 2 * (t - start) / 125;

	start = t;

	return EOK;
}


time_t hal_timerGet(void)
{
	return timer_cycles() / timer_common.ratio;
}


void hal_timerInit(void)
{
	hal_interruptsSet(0, timer_isr, &timer_common);

	/* Calculate cycles to ms ratio */
	while (!timer_common.ratio)
		;

	hal_interruptsSet(0, NULL, NULL);
}
