/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Triple Timer controller
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


/* TODO: this value should be calculated and provide by zynq API */
#define TTC_SRC_CLK_CPU_1x 99990000 /* Hz */
#define TTC_DEFAULT_FREQ   1000     /* Hz */

/* clang-format off */
enum {
	clk_ctrl1 = 0, clk_ctrl2, clk_ctrl3,
	cnt_ctrl1, cnt_ctrl2, cnt_ctrl3,
	cnt_value1, cnt_value2, cnt_value3,
	interval_cnt1, interval_cnt2, interval_cnt3,
	match0_cnt1, match1_cnt2, match1_cnt3,
	match1_cnt1, match2_cnt2, match2_cnt3,
	match2_cnt1, match3_cnt2, match3_cnt3,
	isr1, isr2, isr3,
	ier1, ier2, ier3,
	ev_ctrl_t1, ev_ctrl_t2, ev_ctrl_t3,
	ev_reg1, ev_reg2, ev_reg3
};
/* clang-format on */


static struct {
	volatile u32 *base;
	volatile time_t time;
	u32 ticksPerFreq;
} timer_common;


static int timer_isr(unsigned int irq, void *data)
{
	u32 st;

	st = *(timer_common.base + isr1);

	/* Interval IRQ */
	if (st & 0x1) {
		++timer_common.time;
	}

	/* Clear irq status */
	*(timer_common.base + isr1) = st;

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
		*(timer_common.base + clk_ctrl1) |= (prescaler << 1);
		*(timer_common.base + clk_ctrl1) |= 0x1;
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


static void timer_reset(void)
{
	/* Disable timer */
	*(timer_common.base + clk_ctrl1) = 0;

	/* Reset count control register */
	*(timer_common.base + cnt_ctrl1) = 0x00000021;

	/* Reset registers */
	*(timer_common.base + interval_cnt1) = 0;
	*(timer_common.base + interval_cnt2) = 0;
	*(timer_common.base + interval_cnt3) = 0;
	*(timer_common.base + match0_cnt1) = 0;
	*(timer_common.base + match1_cnt2) = 0;
	*(timer_common.base + match2_cnt3) = 0;
	*(timer_common.base + ier1) = 0;
	*(timer_common.base + isr1) = 0x1f;

	/* Reset counters and restart counting */
	*(timer_common.base + cnt_ctrl1) = 0x10;
}


void timer_done(void)
{
	timer_reset();
	_zynqmp_devReset(ctl_reset_lpd_ttc0, 1);
	hal_interruptsSet(TTC0_1_IRQ, NULL, NULL);
}


void timer_init(void)
{
	timer_common.time = 0;
	timer_common.base = TTC0_BASE_ADDR;

	_zynqmp_devReset(ctl_reset_lpd_ttc0, 0);
	timer_reset();

	/* Trigger interrupt at TTC_DEFAULT_FREQ = 1000 Hz */
	timer_setPrescaler(TTC_DEFAULT_FREQ);

	hal_interruptsSet(TTC0_1_IRQ, timer_isr, NULL);

	*(timer_common.base + interval_cnt1) |= timer_common.ticksPerFreq & 0xffff;

	/* Reset counter */
	*(timer_common.base + cnt_ctrl1) = 0x2;
	/* Enable interval irq timer */
	*(timer_common.base + ier1) = 0x1;
}
