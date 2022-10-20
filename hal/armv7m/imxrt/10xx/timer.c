/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GPT (General Purpose Timer) driver
 *
 * Copyright 2020-2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


enum { gpt_cr, gpt_pr, gpt_sr, gpt_ir, gpt_ocr1, gpt_ocr2, gpt_ocr3, gpt_icr1, gpt_icr2, gpt_cnt };


struct {
	volatile u32 *base;
	volatile time_t time;
} timer_common;


__attribute__((section(".noxip"))) static int timer_isr(unsigned int irq, void *data)
{
	/* Output Compare 1 Interrupt */
	if (*(timer_common.base + gpt_sr) & 0x1) {
		*(timer_common.base + gpt_sr) = 0x1; /* clear status */
		++timer_common.time;
	}

	hal_cpuDataSyncBarrier();

	return 0;
}


__attribute__((section(".noxip"))) time_t hal_timerGet(void)
{
	time_t val;

	hal_interruptsDisable(GPT1_IRQ);
	val = timer_common.time;
	hal_interruptsEnable(GPT1_IRQ);

	return val;
}


void timer_done(void)
{
	/* Reset timer */
	*(timer_common.base + gpt_ir) = 0;
	*(timer_common.base + gpt_cr) &= ~0x1;

	/* Clear status register */
	*(timer_common.base + gpt_sr) = *(timer_common.base + gpt_sr);

	hal_interruptsSet(GPT1_IRQ, NULL, NULL);
}


void timer_init(void)
{
	u32 freq, ticksPerMs;

	timer_common.base = (void *)GPT1_BASE;

	freq = _imxrt_ccmGetFreq(clk_ipg) / 2; /* 66000000 Hz */
	ticksPerMs = freq / 1000;

	_imxrt_setDevClock(GPT1_CLK, clk_state_run);

	*(timer_common.base + gpt_cr) |= 1 << 15;

	while (((*(timer_common.base + gpt_cr) >> 15) & 0x1));

	/* Disable GPT and it's interrupts */
	*(timer_common.base + gpt_cr) = 0;
	*(timer_common.base + gpt_ir) = 0;

	/* Clear status register */
	*(timer_common.base + gpt_sr) = *(timer_common.base + gpt_sr);

	/* Choose peripheral clock as clock source */
	*(timer_common.base + gpt_cr) = 0x1 << 6;

	/* Set enable mode (ENMOD) and Free-Run mode */
	*(timer_common.base + gpt_cr) |= (1 << 1) | (1 << 3) | (1 << 5);

	hal_interruptsSet(GPT1_IRQ, timer_isr, NULL);

	*(timer_common.base + gpt_ocr1) = ticksPerMs;
	*(timer_common.base + gpt_ir) |= 0x1; /* Set OF1IE (Output Compare 1 Interrupt Enable) */

	/* Enable timer*/
	*(timer_common.base + gpt_cr) |= 1;
}
