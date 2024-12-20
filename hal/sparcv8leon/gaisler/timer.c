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

/* Timer registers */

#define GPT_SCALER     0             /* Scaler value register                 : 0x00 */
#define GPT_SRELOAD    1             /* Scaler reload value register          : 0x04 */
#define GPT_CONFIG     2             /* Configuration register                : 0x08 */
#define GPT_LATCHCFG   3             /* Latch configuration register          : 0x0C */
#define GPT_TCNTVAL(n) (n * 4)       /* Timer n counter value reg (n=1,2,...) : 0xn0 */
#define GPT_TRLDVAL(n) ((n * 4) + 1) /* Timer n reload value register         : 0xn4 */
#define GPT_TCTRL(n)   ((n * 4) + 2) /* Timer n control register              : 0xn8 */
#define GPT_TLATCH(n)  ((n * 4) + 3) /* Timer n latch register                : 0xnC */

#define TIMER_DEFAULT 1

#define TIMER_DEFAULT_FREQ 1000


static struct {
	vu32 *gptimer0_base;
	vu32 *gptimer1_base;
	volatile time_t time;
	u32 ticksPerFreq;
} timer_common;


static int timer_isr(unsigned int irq, void *data)
{
	vu32 st = *(timer_common.gptimer0_base + GPT_TCTRL(TIMER_DEFAULT)) & TIMER_INT_PENDING;

	if (st != 0) {
		++timer_common.time;
		/* Clear irq status - set & clear to handle different GPTIMER core versions */
		*(timer_common.gptimer0_base + GPT_TCTRL(TIMER_DEFAULT)) |= TIMER_INT_PENDING;
		*(timer_common.gptimer0_base + GPT_TCTRL(TIMER_DEFAULT)) &= ~TIMER_INT_PENDING;
	}

	return 0;
}


static void timer_setPrescaler(int timer, u32 freq)
{
	u32 prescaler = SYSCLK_FREQ / 1000000; /* 1 MHz */
	u32 ticks = (SYSCLK_FREQ / prescaler) / freq;

	*(timer_common.gptimer0_base + GPT_TRLDVAL(timer)) = ticks - 1;
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
	vu32 st;
	/* Disable timer interrupts - bits cleared when written 1 */
	st = *(timer_common.gptimer0_base + GPT_TCTRL(TIMER_DEFAULT)) & (TIMER_INT_ENABLE | TIMER_INT_PENDING);
	*(timer_common.gptimer0_base + GPT_TCTRL(TIMER_DEFAULT)) = st;

	for (timer = 1; timer <= TIMER0_CNT; ++timer) {
		/* Disable timers */
		*(timer_common.gptimer0_base + GPT_TCTRL(timer)) = 0;
		/* Reset counter and reload value */
		*(timer_common.gptimer0_base + GPT_TCNTVAL(timer)) = 0;
		*(timer_common.gptimer0_base + GPT_TRLDVAL(timer)) = 0;
	}

	/* We might not have second timer core */
	if (timer_common.gptimer1_base != NULL) {
		st = *(timer_common.gptimer1_base + GPT_TCTRL(TIMER_DEFAULT)) & (TIMER_INT_ENABLE | TIMER_INT_PENDING);
		*(timer_common.gptimer1_base + GPT_TCTRL(TIMER_DEFAULT)) = st;

		for (timer = 1; timer <= TIMER1_CNT; ++timer) {
			*(timer_common.gptimer1_base + GPT_TCTRL(TIMER_DEFAULT)) = 0;
			*(timer_common.gptimer1_base + GPT_TCNTVAL(timer)) = 0;
			*(timer_common.gptimer1_base + GPT_TRLDVAL(timer)) = 0;
		}
	}

	hal_interruptsSet(TIMER0_1_IRQ, NULL, NULL);
}


void timer_init(void)
{
	timer_common.time = 0;
	timer_common.gptimer0_base = (u32 *)GPTIMER0_BASE;
	timer_common.gptimer1_base = (u32 *)GPTIMER1_BASE;

	/* Reset timer */
	timer_done();

	timer_setPrescaler(TIMER_DEFAULT, TIMER_DEFAULT_FREQ);

	hal_interruptsSet(TIMER0_1_IRQ, timer_isr, NULL);

	/* Enable timer and interrupts */
	/* Load reload value into counter register */
	*(timer_common.gptimer0_base + GPT_TCTRL(TIMER_DEFAULT)) |= TIMER_ENABLE | TIMER_PERIODIC | TIMER_LOAD | TIMER_INT_ENABLE;
}
