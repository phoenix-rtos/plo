/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Triple Timer controler
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "../low.h"
#include "../timer.h"

#include "peripherals.h"


/* TODO: this value should be calculated and provide by zynq API */
#define TTC_SRC_CLK_CPU_1x   111111115 /* Hz */
#define TTC_DEFAULT_FREQ     1000      /* Hz */


enum {
	clk_ctrl, clk_ctrl2, clk_ctrl3, cnt_ctrl, cnt_ctrl2, cnt_ctrl3, cnt_value, cnt_value2, cnt_value3, interval_val, interval_cnt2, interval_cnt3,
	match0, match1_cnt2, match1_cnt3, match1, match2_cnt2, match2_cnt3, match2, match3_cnt2, match3_cnt3, isr, irq_reg2, irq_reg3, ier, irq_en2,
	irq_en3, ev_ctrl_t1, ev_ctrl_t2, ev_ctrl_t3, ev_reg1, ev_reg2, ev_reg3
};


typedef struct {
	volatile u32 *base;
	volatile u32 done;
	u32 ticksPerFreq;
	u32 leftTime;

	u16 irq;
} timer_t;


timer_t timer_common;


static int timer_isr(u16 irq, void *data)
{
	u32 st;
	timer_t *timer = (timer_t *)data;

	st = *(timer->base + isr);

	/* Interval IRQ */
	if (st & 0x1) {
		if (!(timer->leftTime--)) {
			/* Reset counter */
			*(timer->base + cnt_ctrl) &= ~(1 << 1);

			/* Disable irq */
			*(timer->base + isr) |= 0x1;
			*(timer->base + ier) &= ~0x1;
		}
	}

	/* Clear irq status */
	*(timer->base + isr) = st;

	return 0;
}


static void timer_setPrescaler(u32 freq)
{
	u32 ticks;
	u8 prescaler;

	ticks = TTC_SRC_CLK_CPU_1x / freq;

	/* Prescaler is not needed */
	if (ticks < 0xffff) {
		timer_common.ticksPerFreq = ticks;
		return;
	}

	for (prescaler = 0; prescaler < 0xf; ++prescaler) {
		ticks = TTC_SRC_CLK_CPU_1x / freq / (1 << (prescaler + 1));

		if (ticks < 0xffff)
			break;
	}

	/* Enable and set prescaler */
	*(timer_common.base + clk_ctrl) |= 0x1;
	*(timer_common.base + clk_ctrl) |= (prescaler & 0xf) << 1;

	timer_common.ticksPerFreq = ticks;
}


/* TODO */
void timer_cycles(u64 *c)
{

}


/* TODO */
void timer_cyclesdiff(u64 *c1, u64 *c2, u64 *res)
{

}


int timer_wait(u32 ms, int flags, u16 *p, u16 v)
{
	/* Set value that determines when an irq t will be generated */
	timer_common.leftTime = ms;
	*(timer_common.base + interval_val) |= timer_common.ticksPerFreq & 0xffff;

	/* Reset counter */
	*(timer_common.base + cnt_ctrl) = 0x2;
	/* Enable interval irq timer */
	*(timer_common.base + ier) = 0x1;

	for (;;) {
		if (!timer_common.leftTime)
			break;

		if (((flags & TIMER_KEYB) && low_keypressed()) ||
			((flags & TIMER_VALCHG) && *p != v))
			return 1;
	}

	return 0;
}


void timer_init(void)
{
	timer_common.irq = TTC0_1_IRQ;
	timer_common.base = TTC0_BASE_ADDR;

	/* Reset controller */
	timer_done();

	/* Trigger interrupt at TTC_DEFAULT_FREQ = 1000 Hz */
	timer_setPrescaler(TTC_DEFAULT_FREQ);

	low_irqinst(timer_common.irq, timer_isr, (void *)&timer_common);
}


void timer_done(void)
{
	/* Disable timer */
	*(timer_common.base + clk_ctrl) = 0;

	/* Reset count control register */
	*(timer_common.base + cnt_ctrl) = 0x00000021;

	/* Reset registers */
	*(timer_common.base + interval_val) = 0;
	*(timer_common.base + interval_cnt2) = 0;
	*(timer_common.base + interval_cnt3) = 0;
	*(timer_common.base + match0) = 0;
	*(timer_common.base + match1_cnt2) = 0;
	*(timer_common.base + match2_cnt3) = 0;
	*(timer_common.base + ier) = 0;
	*(timer_common.base + isr) = 0x1f;

	/* Reset counters and restart counting */
	*(timer_common.base + cnt_ctrl) = 0x10;

	low_irquninst(timer_common.irq);
}
