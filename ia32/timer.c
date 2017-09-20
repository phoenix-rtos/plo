/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * System timer driver
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "low.h"
#include "plostd.h"
#include "timer.h"


typedef struct {
	u16 jiffies;
} timer_t;


timer_t timer;


int timer_isr(u16 irq, void *data)
{
	timer_t *timer = data;

	timer->jiffies += 62;
	return IRQ_DEFAULT;
}


int timer_wait(u16 ms, int flags, u16 *p, u16 v)
{
	u16 current, dt;

	current = timer.jiffies;

	for (;;) {
		if (current > timer.jiffies)
			dt = 0xffff - current + timer.jiffies;
		else
			dt = timer.jiffies - current;

		if (ms && (dt >= ms))
			break;

		if (((flags & TIMER_KEYB) && low_keypressed()) ||
				((flags & TIMER_VALCHG) && (*p != v)))
			return 1;
	}
	return 0;
}


void timer_init(void)
{
	timer.jiffies = 0;
	low_irqinst(0, timer_isr, (void *)&timer);
	return;
}


void timer_done(void)
{
	low_irquninst(0);
	low_irquninst(1);
}
