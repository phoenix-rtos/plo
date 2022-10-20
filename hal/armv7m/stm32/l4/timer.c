/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Timer driver
 *
 * Copyright 2021 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "stm32l4.h"

#define SYSTICK_IRQ 15

struct {
	volatile time_t time;
	unsigned int interval;
} timer_common;


static int timer_isr(unsigned int irq, void *data)
{
	(void)irq;
	(void)data;

	timer_common.time += (timer_common.interval + 500) / 1000;
	hal_cpuDataSyncBarrier();
	return 0;
}


time_t hal_timerGet(void)
{
	time_t val;

	hal_interruptsDisableAll();
	val = timer_common.time;
	hal_interruptsEnableAll();

	return val;
}


void timer_done(void)
{
	_stm32_systickDone();
	hal_interruptsSet(SYSTICK_IRQ, NULL, NULL);
}


void timer_init(void)
{
	timer_common.time = 0;
	timer_common.interval = 1000;
	_stm32_systickInit(timer_common.interval);
	hal_interruptsSet(SYSTICK_IRQ, timer_isr, NULL);
}
