/*
 * Phoenix-RTOS
 *
 * Operating system loader
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

#include <hal/hal.h>


/* TODO: this value should be calculated and provide by zynq API */
#define TTC_SRC_CLK_CPU_1x 111111115 /* Hz */
#define TTC_DEFAULT_FREQ   1000      /* Hz */


enum {
	clk_ctrl = 0, clk_ctrl2, clk_ctrl3, cnt_ctrl, cnt_ctrl2, cnt_ctrl3, cnt_value, cnt_value2, cnt_value3, interval_val, interval_cnt2, interval_cnt3,
	match0, match1_cnt2, match1_cnt3, match1, match2_cnt2, match2_cnt3, match2, match3_cnt2, match3_cnt3, isr, irq_reg2, irq_reg3, ier, irq_en2,
	irq_en3, ev_ctrl_t1, ev_ctrl_t2, ev_ctrl_t3, ev_reg1, ev_reg2, ev_reg3
};


struct {
	volatile u32 *base;
	volatile time_t time;
	u32 ticksPerFreq;
} timer_common;


static int timer_isr(unsigned int irq, void *data)
{
	u32 st;

	st = *(timer_common.base + isr);

	/* Interval IRQ */
	if (st & 0x1)
		++timer_common.time;

	/* Clear irq status */
	*(timer_common.base + isr) = st;

	return 0;
}


static void timer_setPrescaler(u32 freq)
{
	u32 ticks;
	u32 prescaler;

	ticks = TTC_SRC_CLK_CPU_1x / freq;

	prescaler = 0;
	while ((ticks >= 0xffff) && (prescaler < 0x10)) {
		prescaler++;
		ticks /= 2;
	}

	if (prescaler) {
		/* Enable and set prescaler */
		prescaler--;
		*(timer_common.base + clk_ctrl) |= (prescaler << 1);
		*(timer_common.base + clk_ctrl) |= 0x1;
	}

	timer_common.ticksPerFreq = ticks;
}


time_t hal_timerGet(void)
{
	time_t val;

	hal_interruptsDisable(TTC0_1_IRQ);
	val = timer_common.time;
	hal_interruptsEnable(TTC0_1_IRQ);

	return val;
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

	hal_interruptsSet(TTC0_1_IRQ, NULL, NULL);
}


void timer_init(void)
{
	timer_common.time = 0;
	timer_common.base = TTC0_BASE_ADDR;

	/* Reset controller */
	timer_done();

	/* Trigger interrupt at TTC_DEFAULT_FREQ = 1000 Hz */
	timer_setPrescaler(TTC_DEFAULT_FREQ);

	hal_interruptsSet(TTC0_1_IRQ, timer_isr, NULL);

	*(timer_common.base + interval_val) |= timer_common.ticksPerFreq & 0xffff;

	/* Reset counter */
	*(timer_common.base + cnt_ctrl) = 0x2;
	/* Enable interval irq timer */
	*(timer_common.base + ier) = 0x1;
}
