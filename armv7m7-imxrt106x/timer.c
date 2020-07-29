/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * GPT (General Purpose Timer) driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../low.h"
#include "../plostd.h"
#include "../timer.h"

#include "imxrt.h"
#include "config.h"


enum { gpt_cr, gpt_pr, gpt_sr, gpt_ir, gpt_ocr1, gpt_ocr2, gpt_ocr3, gpt_icr1, gpt_icr2, gpt_cnt };


typedef struct {
    volatile u32 *base;
    volatile u32 done;
    u32 ticksPerMs;

    u16 irq;
} timer_t;


timer_t timer_common;


static int timer_isr(u16 irq, void *data)
{
    timer_t *timer = (timer_t *)data;

    *(timer->base + gpt_sr) = 0x1;
    *(timer->base + gpt_ir) = 0x1;

    timer_common.done = 1;

    imxrt_dataSyncBarrier();

    return 0;
}


void timer_cycles(u64 *c)
{
    return;
}


void timer_cyclesdiff(u64 *c1, u64 *c2, u64 *res)
{
    return;
}


int timer_wait(u32 ms, int flags, u16 *p, u16 v)
{
    /* Set value that determines when a compare event will be generated */
    *(timer_common.base + gpt_ocr1) = timer_common.ticksPerMs * ms;

    *(timer_common.base + gpt_ir) |= 0x1; /* Set OF1IE (Output Compare 1 Interrupt Enable) */

    timer_common.done = 0;

    /* Enable timer*/
    *(timer_common.base + gpt_cr) |= 1;

    for (;;) {
        if (timer_common.done)
            break;

        if (((flags & TIMER_KEYB) && low_keypressed()) ||
            ((flags & TIMER_VALCHG) && *p != v))
            return 1;
    }

    /* Disable timer */
    *(timer_common.base + gpt_cr) |= 0;

    return 0;
}


void timer_init(void)
{
    u32 freq;

    timer_common.base = (void *) GPT1_BASE;
    timer_common.irq = GPT1_IRQ;

    freq = _imxrt_ccmGetFreq(clk_ipg) / 2;  /* 66000000 Hz */
    timer_common.ticksPerMs = freq / 1000;

    _imxrt_setDevClock(GPT1_CLK, clk_state_run);

    *(timer_common.base + gpt_cr) |= 1 << 15;

    while (((*(timer_common.base + gpt_cr) >> 15) & 0x1))
    { }

    /* Disable GPT and it's interrupts */
    *(timer_common.base + gpt_cr) = 0;
    *(timer_common.base + gpt_ir) = 0;

    /* Clear status register */
    *(timer_common.base + gpt_sr) = *(timer_common.base + gpt_sr);

    /* Choose peripheral clock as clock source */
    *(timer_common.base + gpt_cr) = 0x1 << 6;

    /* Set enable mode (ENMOD) and Free-Run mode */
    *(timer_common.base + gpt_cr) |= (1 << 1) | (1 << 3) | (1 << 5);

    low_irqinst(GPT1_IRQ, timer_isr, (void *)&timer_common);

    return;
}


void timer_done(void)
{
    low_irquninst(timer_common.irq);
}
