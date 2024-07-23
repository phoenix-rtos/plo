/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARM Dual Timer driver
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

/* Timer registers */
/* clang-format off */
enum {
	timer1_load = 0, timer1_value, timer1_ctrl, timer1_intclr, timer1_ris, timer1_mis, timer1_bgload,
	timer2_load = 8, timer2_value, timer2_ctrl, timer2_intclr, timer2_ris, timer2_mis, timer2_bgload
};
/* clang-format on */


static struct {
	volatile u32 *base;
	volatile time_t time;
} timer_common = {
	.base = (volatile u32 *)TIMER_BASE
};


static int timer_isr(unsigned int irq, void *data)
{
	(void)irq;
	(void)data;

	if ((*(timer_common.base + timer1_mis) & 0x1) != 0) {
		*(timer_common.base + timer1_intclr) = 0;
		timer_common.time++;
		hal_cpuDataSyncBarrier();
	}

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
	hal_interruptsSet(TIMER_IRQ, NULL, NULL);

	/* Disable timer */
	*(timer_common.base + timer1_ctrl) &= ~(1 << 7);
}


void timer_init(void)
{
	/* Reset timer */
	*(timer_common.base + timer1_ctrl) &= ~(1 << 7);
	*(timer_common.base + timer1_value) = 0;

	/* Periodic mode, 32-bit, enable interrupt */
	*(timer_common.base + timer1_ctrl) = (1 << 6) | (1 << 5) | (1 << 1);
	hal_cpuDataSyncBarrier();
	*(timer_common.base + timer1_load) = (SYSCLK_FREQ / 1000) - 1;
	hal_cpuDataSyncBarrier();

	hal_interruptsSet(TIMER_IRQ, timer_isr, NULL);

	/* Enable timer */
	*(timer_common.base + timer1_ctrl) |= (1 << 7);
}
