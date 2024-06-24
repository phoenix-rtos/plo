/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Timer driver
 *
 * Copyright 2024 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "n94x.h"


/* clang-format off */
enum { ostimer_evtimerl = 0, ostimer_evtimerh, ostimer_capturel, ostimer_captureh,
	ostimer_matchl, ostimer_matchh, ostimer_oseventctrl = 7 };
/* clang-format on */


static struct {
	volatile u32 *base;
	u32 high;
	u64 timerLast;
} timer_common;


static u64 timer_gray2bin(u64 gray)
{
	return _mcxn94x_sysconGray2Bin(gray);
}

static time_t hal_timerCyc2Us(time_t ticks)
{
	return (ticks * 1000 * 1000) / 32768;
}


static u64 hal_timerGetCyc(void)
{
	u32 high = *(timer_common.base + ostimer_evtimerh);
	u32 low = *(timer_common.base + ostimer_evtimerl);
	u64 timerval;

	if (high != *(timer_common.base + ostimer_evtimerh)) {
		/* Rollover, read again */
		high = *(timer_common.base + ostimer_evtimerh);
		low = *(timer_common.base + ostimer_evtimerl);
	}

	timerval = timer_gray2bin(low | ((u64)high << 32));
	if (timerval < timer_common.timerLast) {
		/* Once every ~4 years */
		timer_common.high += 1 << (42 - 32);
	}
	timer_common.timerLast = timerval;

	return timerval | timer_common.high;
}


time_t hal_timerGet(void)
{
	return hal_timerCyc2Us(hal_timerGetCyc()) / 1000;
}


void timer_done(void)
{
	/* Disable and reset the timer */
	_mcxn94x_sysconDevReset(pctl_ostimer, 1);
}


void timer_init(void)
{
	timer_common.base = (void *)0x40049000;
	timer_common.timerLast = 0;
	timer_common.high = 0;

	/* Configure OSTIMER clock */
	/* Use xtal32k clock source, enable the clock, deassert reset */
	_mcxn94x_sysconSetDevClk(pctl_ostimer, 1, 0, 1);
	_mcxn94x_sysconDevReset(pctl_ostimer, 0);
}
