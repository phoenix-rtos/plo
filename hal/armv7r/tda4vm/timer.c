/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * OMAP5430 Timer
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#define TIMER_INTR_OVERFLOW (1 << 1)
#define TIMER_MAX_COUNT     0xffffffffu

enum timer_regs {
	timer_tidr = (0x0 / 4),           /* Revision register */
	timer_tiocp_cfg = (0x10 / 4),     /* CBASS0 Configuration register */
	timer_irq_eoi = (0x20 / 4),       /* End-Of-Interrupt register */
	timer_irqstatus_raw = (0x24 / 4), /* Timer raw status register */
	timer_irqstatus = (0x28 / 4),     /* Timer status register */
	timer_irqstatus_set = (0x2c / 4), /* Interrupt enable register */
	timer_irqstatus_clr = (0x30 / 4), /* Interrupt disable register */
	timer_irqwakeen = (0x34 / 4),     /* Wake-up enable register */
	timer_tclr = (0x38 / 4),          /* Timer control register */
	timer_tcrr = (0x3c / 4),          /* Timer counter register */
	timer_tldr = (0x40 / 4),          /* Timer load register */
	timer_ttgr = (0x44 / 4),          /* Timer trigger register */
	timer_twps = (0x48 / 4),          /* Timer write-posted register */
	timer_tmar = (0x4c / 4),          /* Timer match register */
	timer_tcar1 = (0x50 / 4),         /* First captured value of the timer counter */
	timer_tsicr = (0x54 / 4),         /* Timer synchronous interface control register */
	timer_tcar2 = (0x58 / 4),         /* Second captured value of the timer counter */
	timer_tpir = (0x5c / 4),          /* Timer positive increment register */
	timer_tnir = (0x60 / 4),          /* Timer negative increment register */
	timer_tcvr = (0x64 / 4),          /* Timer CVR counter register */
	timer_tocr = (0x68 / 4),          /* Timer overflow counter register */
	timer_towr = (0x6c / 4),          /* Timer overflow wrapping register */
};


static struct {
	volatile u32 *base;
	volatile time_t time;
	u32 ticksPerFreq;
} timer_common;


static int timer_isr(unsigned int irq, void *data)
{
	/* On TDA4VM timer interrupts are level triggered - don't use timer_irq_eoi */
	u32 st;

	st = *(timer_common.base + timer_irqstatus);

	/* Overflow IRQ */
	if ((st & (1 << 1)) != 0) {
		++timer_common.time;
	}

	/* Clear IRQ status */
	*(timer_common.base + timer_irqstatus) = st;

	return 0;
}


static void timer_setPrescaler(u32 freq)
{
	u64 ticks;
	u32 prescaler;

	ticks = TIMER_SRC_FREQ_HZ / freq;

	prescaler = 0;
	while ((ticks >= TIMER_MAX_COUNT) && (prescaler < 8)) {
		prescaler++;
		ticks /= 2;
	}

	if (prescaler != 0) {
		/* Enable and set prescaler */
		prescaler--;
		*(timer_common.base + timer_tclr) |= (prescaler << 2);
		*(timer_common.base + timer_tclr) |= (1 << 5);
	}

	timer_common.ticksPerFreq = ticks;
}


time_t hal_timerGet(void)
{
	time_t val;

	hal_interruptsDisable(MCU_TIMER0_INTR);
	val = timer_common.time;
	hal_interruptsEnable(MCU_TIMER0_INTR);

	return val;
}


static void timer_reset(void)
{
	/* Stop timer, set autoreload on, all other bits to 0 */
	*(timer_common.base + timer_tclr) = (1 << 1);
}


void timer_done(void)
{
	hal_interruptsSet(MCU_TIMER0_INTR, NULL, NULL);
	timer_reset();
}


void timer_init(void)
{
	timer_common.time = 0;
	timer_common.base = MCU_TIMER_BASE_ADDR(0);
	timer_reset();

	tda4vm_setClksel(clksel_mcu_timer0, 1); /* Source: MCU_SYSCLK0 / 4 */
	/* Trigger interrupt at TIMER_TICK_HZ = 1000 Hz */
	timer_setPrescaler(TIMER_TICK_HZ);

	hal_interruptsSet(MCU_TIMER0_INTR, timer_isr, NULL);

	/* Timer will be reloaded with timer_tldr value on overflow, so subtract
	 * number of ticks per timer overflow from its maximum value */
	*(timer_common.base + timer_tldr) = (TIMER_MAX_COUNT - timer_common.ticksPerFreq) + 1;
	*(timer_common.base + timer_ttgr) = 0; /* Write any value to reload counter */

	/* Start counting */
	*(timer_common.base + timer_tclr) |= 1;

	/* Enable overflow IRQ */
	*(timer_common.base + timer_irqstatus_set) = (1 << 1);
}
