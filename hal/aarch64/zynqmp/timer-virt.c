/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Triple Timer controller
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

static struct {
	u32 freq;
} timer_common;

time_t hal_timerGet(void)
{
	u64 ticks;

	/* Read the ARM Generic Timer Physical Counter */
	__asm__ volatile("mrs %0, cntpct_el0" : "=r"(ticks));

	if (timer_common.freq == 0) {
		return 0; /* Safety check if called before init */
	}

	return (time_t)((ticks * 1000ULL) / timer_common.freq);
}


void timer_done(void)
{
}


void timer_init(void)
{
	u64 freq;

	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));

	timer_common.freq = (u32)freq;

}
