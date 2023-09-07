/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Timer controller
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

/* CLINT Timer interrupt */
#define TIMER_IRQ (5u | CLINT_IRQ_FLG)


static struct {
	volatile time_t jiffies;
	u32 interval;
} timer_common;


static int timer_irqHandler(unsigned int n, void *arg)
{
	(void)n;
	(void)arg;

	timer_common.jiffies++;
	sbi_setTimer(csr_read(time) + timer_common.interval);

	return 0;
}


time_t hal_timerGet(void)
{
	time_t val;

	hal_interruptsDisable(TIMER_IRQ);
	val = timer_common.jiffies;
	hal_interruptsEnable(TIMER_IRQ);

	return val;
}


void timer_init(void)
{
	timer_common.jiffies = 0;
	timer_common.interval = TIMER_FREQ / 1000; /* 1ms */

	hal_interruptsSet(TIMER_IRQ, timer_irqHandler, NULL);
	sbi_setTimer(csr_read(time) + timer_common.interval);
}
