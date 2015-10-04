/* 
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * System timer driver
 *
 * Copyright 2001, 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * Phoenix-RTOS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Phoenix-RTOS kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phoenix-RTOS kernel; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

