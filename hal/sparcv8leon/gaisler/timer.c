/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Timer controller
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

/* Timer control bitfields */

#define TIMER_ENABLE      (1 << 0)
#define TIMER_PERIODIC    (1 << 1)
#define TIMER_LOAD        (1 << 2)
#define TIMER_INT_ENABLE  (1 << 3)
#define TIMER_INT_PENDING (1 << 4)
#define TIMER_CHAIN       (1 << 5)

#define TIMER_DEFAULT_FREQ 1000


/* clang-format off */

enum { timer1 = 0, timer2, timer3, timer4 };

/* clang-format on */


static struct {
	vu32 *gptimer0_base;
	vu32 *gptimer1_base;
	volatile time_t time;
	u32 ticksPerFreq;
} timer_common;


static int timer_isr(unsigned int irq, void *data)
{
	vu32 st = *(timer_common.gptimer0_base + GPT_TCTRL1) & TIMER_INT_PENDING;

	if (st != 0) {
		++timer_common.time;
		/* Clear irq status - set & clear to handle different GPTIMER core versions */
		*(timer_common.gptimer0_base + GPT_TCTRL1) |= TIMER_INT_PENDING;
		*(timer_common.gptimer0_base + GPT_TCTRL1) &= ~TIMER_INT_PENDING;
	}

	return 0;
}


static void timer_setPrescaler(int timer, u32 freq)
{
	u32 prescaler = SYSCLK_FREQ / 1000000; /* 1 MHz */
	u32 ticks = (SYSCLK_FREQ / prescaler) / freq;

	*(timer_common.gptimer0_base + GPT_TRLDVAL1 + timer * 4) = ticks - 1;
	*(timer_common.gptimer0_base + GPT_SRELOAD) = prescaler - 1;

	timer_common.ticksPerFreq = ticks;
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
	int timer;
	/* Disable timer interrupts - bits cleared when written 1 */
	vu32 st = *(timer_common.gptimer0_base + GPT_TCTRL1) & (TIMER_INT_ENABLE | TIMER_INT_PENDING);
	*(timer_common.gptimer0_base + GPT_TCTRL1) = st;
	st = *(timer_common.gptimer1_base + GPT_TCTRL1) & (TIMER_INT_ENABLE | TIMER_INT_PENDING);
	*(timer_common.gptimer1_base + GPT_TCTRL1) = st;

	for (timer = 0; timer < TIMER0_CNT; ++timer) {
		/* Disable timers */
		*(timer_common.gptimer0_base + GPT_TCTRL1 + timer * 4) = 0;
		/* Reset counter and reload value */
		*(timer_common.gptimer0_base + GPT_TCNTVAL1 + timer * 4) = 0;
		*(timer_common.gptimer0_base + GPT_TRLDVAL1 + timer * 4) = 0;
	}

	for (timer = 0; timer < TIMER1_CNT; ++timer) {
		*(timer_common.gptimer1_base + GPT_TCTRL1 + timer * 4) = 0;
		*(timer_common.gptimer1_base + GPT_TCNTVAL1 + timer * 4) = 0;
		*(timer_common.gptimer1_base + GPT_TRLDVAL1 + timer * 4) = 0;
	}

	hal_interruptsSet(TIMER_IRQ, NULL, NULL);
}


void timer_init(void)
{
	timer_common.time = 0;
	timer_common.gptimer0_base = (u32 *)GPTIMER0_BASE;
	timer_common.gptimer1_base = (u32 *)GPTIMER1_BASE;

	/* Reset timer */
	timer_done();

	timer_setPrescaler(timer1, TIMER_DEFAULT_FREQ);

	hal_interruptsSet(TIMER_IRQ, timer_isr, NULL);

	/* Enable timer and interrupts */
	/* Load reload value into counter register */
	*(timer_common.gptimer0_base + GPT_TCTRL1) |= TIMER_ENABLE | TIMER_PERIODIC | TIMER_LOAD | TIMER_INT_ENABLE;
}
