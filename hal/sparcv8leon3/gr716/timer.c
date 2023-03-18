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
#define TIMER_ENABLE      (1)
#define TIMER_PERIODIC    (1 << 1)
#define TIMER_LOAD        (1 << 2)
#define TIMER_INT_ENABLE  (1 << 3)
#define TIMER_INT_PENDING (1 << 4)
#define TIMER_CHAIN       (1 << 5)

#define TIMER_DEFAULT_FREQ 1000


enum {
	gpt_scaler = 0, /* Scaler value register        : 0x00 */
	gpt_sreload,    /* Scaler reload value register : 0x04 */
	gpt_config,     /* Configuration register       : 0x08 */
	gpt_latchcfg,   /* Latch configuration register : 0x0C */
	gpt_tcntval1,   /* Timer 1 counter value reg    : 0x10 */
	gpt_trldval1,   /* Timer 1 reload value reg     : 0x14 */
	gpt_tctrl1,     /* Timer 1 control register     : 0x18 */
	gpt_tlatch1,    /* Timer 1 latch register       : 0x1C */
	gpt_tcntval2,   /* Timer 2 counter value reg    : 0x20 */
	gpt_trldval2,   /* Timer 2 reload value reg     : 0x24 */
	gpt_tctrl2,     /* Timer 2 control register     : 0x28 */
	gpt_tlatch2,    /* Timer 2 latch register       : 0x2C */
	gpt_tcntval3,   /* Timer 3 counter value reg    : 0x30 */
	gpt_trldval3,   /* Timer 3 reload value reg     : 0x34 */
	gpt_tctrl3,     /* Timer 3 control register     : 0x38 */
	gpt_tlatch3,    /* Timer 3 latch register       : 0x3C */
	gpt_tcntval4,   /* Timer 4 counter value reg    : 0x40 */
	gpt_trldval4,   /* Timer 4 reload value reg     : 0x44 */
	gpt_tctrl4,     /* Timer 4 control register     : 0x48 */
	gpt_tlatch4,    /* Timer 4 latch register       : 0x4C */
	gpt_tcntval5,   /* Timer 5 counter value reg    : 0x50 */
	gpt_trldval5,   /* Timer 5 reload value reg     : 0x54 */
	gpt_tctrl5,     /* Timer 5 control register     : 0x58 */
	gpt_tlatch5,    /* Timer 5 latch register       : 0x5C */
	gpt_tcntval6,   /* Timer 6 counter value reg    : 0x60 */
	gpt_trldval6,   /* Timer 6 reload value reg     : 0x64 */
	gpt_tctrl6,     /* Timer 6 control register     : 0x68 */
	gpt_tlatch6,    /* Timer 6 latch register       : 0x6C */
	gpt_tcntval7,   /* Timer 7 counter value reg    : 0x70 */
	gpt_trldval7,   /* Timer 7 reload value reg     : 0x74 */
	gpt_tctrl7,     /* Timer 7 control register     : 0x78 */
	gpt_tlatch7,    /* Timer 7 latch register       : 0x7C */
};

/* clang-format off */

enum { timer1 = 0, timer2, timer3, timer4, timer5, timer6, timer7 };

/* clang-format on */

static struct {
	vu32 *gptimer0_base;
	vu32 *gptimer1_base;
	volatile time_t time;
	u32 ticksPerFreq;
} timer_common;


static int timer_isr(unsigned int irq, void *data)
{
	vu32 st = *(timer_common.gptimer0_base + gpt_tctrl1) & TIMER_INT_PENDING;

	if (st != 0) {
		++timer_common.time;
		/* Clear irq status */
		*(timer_common.gptimer0_base + gpt_tctrl1) |= TIMER_INT_PENDING;
	}

	return 0;
}


static void timer_setPrescaler(int timer, u32 freq)
{
	u32 prescaler = SYSCLK_FREQ / 1000000; /* 1 MHz */
	u32 ticks = (SYSCLK_FREQ / prescaler) / freq;

	*(timer_common.gptimer0_base + gpt_trldval1 + timer * 4) = ticks - 1;
	*(timer_common.gptimer0_base + gpt_sreload) = prescaler - 1;

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
	vu32 st = *(timer_common.gptimer0_base + gpt_tctrl1) & (TIMER_INT_ENABLE | TIMER_INT_PENDING);
	*(timer_common.gptimer0_base + gpt_tctrl1) = st;
	st = *(timer_common.gptimer1_base + gpt_tctrl1) & (TIMER_INT_ENABLE | TIMER_INT_PENDING);
	*(timer_common.gptimer1_base + gpt_tctrl1) = st;

	for (timer = timer1; timer <= timer7; ++timer) {
		/* Disable timers */
		*(timer_common.gptimer0_base + gpt_tctrl1 + timer * 4) = 0;
		*(timer_common.gptimer1_base + gpt_tctrl1 + timer * 4) = 0;
		/* Reset counter and reload value */
		*(timer_common.gptimer0_base + gpt_tcntval1 + timer * 4) = 0;
		*(timer_common.gptimer0_base + gpt_trldval1 + timer * 4) = 0;
		*(timer_common.gptimer1_base + gpt_tcntval1 + timer * 4) = 0;
		*(timer_common.gptimer1_base + gpt_trldval1 + timer * 4) = 0;
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
	*(timer_common.gptimer0_base + gpt_tctrl1) |= TIMER_ENABLE | TIMER_PERIODIC | TIMER_LOAD | TIMER_INT_ENABLE;
}
