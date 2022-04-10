/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Timer driver
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "nrf91.h"


/* based on peripheral id table */
#define TIMER_IRQ (timer0 + 16 + TIMER_INSTANCE)


struct {
	volatile u32 *base[3];
	volatile time_t time;
	unsigned int interval;
} timer_common;


/* clang-format off */
enum { timer_tasks_start = 0, timer_tasks_stop, timer_tasks_count, timer_tasks_clear, timer_tasks_shutdown,
	timer_tasks_capture0 = 16, timer_tasks_capture1, timer_tasks_capture2, timer_tasks_capture3, timer_tasks_capture4, timer_tasks_capture5,
	timer_events_compare0 = 80, timer_events_compare1, timer_events_compare2, timer_events_compare3, timer_events_compare4, timer_events_compare5,
	timer_intenset = 193, timer_intenclr, timer_mode = 321, timer_bitmode, timer_prescaler = 324,
	timer_cc0 = 336, timer_cc1, timer_cc2, timer_cc3, timer_cc4, timer_cc5 };
/* clang-format on */


static void timer_clearEvent(void)
{
	/* Clear compare event */
	*(timer_common.base[TIMER_INSTANCE] + timer_events_compare0) = 0u;
	/* Clear counter */
	*(timer_common.base[TIMER_INSTANCE] + timer_tasks_clear) = 1u;
}


static int timer_isr(unsigned int irq, void *data)
{
	(void)irq;
	(void)data;

	timer_clearEvent();
	timer_common.time += 1;
	hal_cpuDataSyncBarrier();
	return 0;
}


time_t hal_timerGet(void)
{
	time_t val;

	hal_interruptsDisable(TIMER_IRQ);
	val = timer_common.time;
	hal_interruptsEnable(TIMER_IRQ);

	return val;
}


void timer_done(void)
{
	/* Stop timer */
	*(timer_common.base[TIMER_INSTANCE] + timer_tasks_stop) = 1u;
	hal_cpuDataMemoryBarrier();
	/* Disable all timer0 interrupts */
	*(timer_common.base[TIMER_INSTANCE] + timer_intenclr) = 0xffffffffu;
	/* Clear timer0 */
	*(timer_common.base[TIMER_INSTANCE] + timer_tasks_clear) = 1u;
	hal_cpuDataMemoryBarrier();

	hal_interruptsSet(TIMER_IRQ, NULL, NULL);
}


void timer_init(void)
{
	timer_common.base[0] = (void *)0x5000f000u;
	timer_common.base[1] = (void *)0x50010000u;
	timer_common.base[2] = (void *)0x50011000u;
	timer_common.time = 0;
	timer_common.interval = 1000;

	/* Stop timer0 */
	*(timer_common.base[TIMER_INSTANCE] + timer_tasks_stop) = 1u;
	hal_cpuDataMemoryBarrier();
	/* Set mode to timer */
	*(timer_common.base[TIMER_INSTANCE] + timer_mode) = 0u;
	/* Set 16-bit mode */
	*(timer_common.base[TIMER_INSTANCE] + timer_bitmode) = 0u;
	/* 1 tick per 1 us */
	*(timer_common.base[TIMER_INSTANCE] + timer_prescaler) = 4u;
	/* 1 compare event per 1ms */
	*(timer_common.base[TIMER_INSTANCE] + timer_cc0) = 1000u;
	/* Enable interrupts from compare0 events */
	*(timer_common.base[TIMER_INSTANCE] + timer_intenset) = 0x10000u;
	/* Clear timer0 */
	*(timer_common.base[TIMER_INSTANCE] + timer_tasks_clear) = 1u;
	hal_cpuDataMemoryBarrier();
	/* Start timer0 */
	*(timer_common.base[TIMER_INSTANCE] + timer_tasks_start) = 1u;
	hal_cpuDataMemoryBarrier();

	hal_interruptsSet(TIMER_IRQ, timer_isr, NULL);
}
