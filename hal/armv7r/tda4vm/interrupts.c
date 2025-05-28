/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * VIM (Vectored Interrupt Manager) driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#define VIM_BASE_ADDRESS 0x40F80000
#define SIZE_INTERRUPTS  384

#define DEFAULT_PRIORITY 7

enum {
	vim_pid = 0,                     /* Revision register */
	vim_info,                        /* Info register */
	vim_priirq,                      /* Prioritized IRQ register */
	vim_prifiq,                      /* Prioritized FIQ register */
	vim_irqgsts,                     /* IRQ group status register */
	vim_fiqgsts,                     /* FIQ group status register */
	vim_irqvec,                      /* IRQ vector address register */
	vim_fiqvec,                      /* FIQ vector address register */
	vim_actirq,                      /* Active IRQ register */
	vim_actfiq,                      /* Active FIQ register */
	vim_dedvec = 12,                 /* DED vector address register */
	vim_raw_m = (0x400 / 4),         /* Raw status/set register */
	vim_sts_m = (0x404 / 4),         /* Interrupt enable status/clear register */
	vim_intr_en_set_m = (0x408 / 4), /* Interrupt enable set register */
	vim_intr_en_clr_m = (0x40c / 4), /* Interrupt enabled clear register */
	vim_irqsts_m = (0x410 / 4),      /* IRQ interrupt enable status/clear register */
	vim_fiqsts_m = (0x414 / 4),      /* FIQ interrupt enable status/clear register */
	vim_intmap_m = (0x418 / 4),      /* Interrupt map register */
	vim_inttype_m = (0x41c / 4),     /* Interrupt type register */
	vim_pri_int_n = (0x1000 / 4),    /* Interrupt priority register */
	vim_vec_int_n = (0x2000 / 4),    /* Interrupt vector register */
};


typedef struct {
	void *data;
	int (*f)(unsigned int, void *);
} intr_handler_t;


static struct {
	volatile u32 *vim;
	intr_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


void hal_interruptsEnable(unsigned int irqn)
{
	unsigned int irq_reg = (irqn / 32) * 8;
	unsigned int irq_offs = irqn % 32;

	if (irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.vim + vim_intr_en_set_m + irq_reg) = 1u << irq_offs;
	hal_cpuDataMemoryBarrier();
}


void hal_interruptsDisable(unsigned int irqn)
{
	unsigned int irq_reg = (irqn / 32) * 8;
	unsigned int irq_offs = irqn % 32;

	if (irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.vim + vim_intr_en_clr_m + irq_reg) = 1u << irq_offs;
	hal_cpuDataMemoryBarrier();
}


static void interrupts_clearStatus(unsigned int irqn)
{
	unsigned int irq_reg = (irqn / 32) * 8;
	unsigned int irq_offs = irqn % 32;

	if (irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.vim + vim_irqsts_m + irq_reg) = 1u << irq_offs;
}


static void interrupts_setPriority(unsigned int irqn, u32 priority)
{
	if (irqn >= SIZE_INTERRUPTS) {
		return;
	}

	*(interrupts_common.vim + vim_pri_int_n + irqn) = priority & 0xf;
}


static inline u32 interrupts_getPriority(unsigned int irqn)
{
	if (irqn >= SIZE_INTERRUPTS) {
		return 0;
	}

	return *(interrupts_common.vim + vim_pri_int_n + irqn) & 0xf;
}


void interrupts_dispatch(void)
{
	int (*f)(unsigned int, void *);
	u32 dummy, irq_val, n;

	/* This register is supposed to be used for ISR vector (pointer to code),
	 * but because lowest 2 bits are hardwired to 0, it cannot store Thumb code pointers.
	 * For this reason we only do dummy read from it and get ISR pointer from the table. */
	dummy = *(interrupts_common.vim + vim_irqvec);
	(void)dummy;

	irq_val = *(interrupts_common.vim + vim_actirq);
	if ((irq_val & (1 << 31)) == 0) {
		/* No interrupt is pending */
		return;
	}

	n = irq_val & 0x3ff;
	if (n >= SIZE_INTERRUPTS) {
		/* This shouldn't happen, but behave in a sane way if it does */
		*(interrupts_common.vim + vim_irqvec) = 0; /* Write any value */
		return;
	}

	f = interrupts_common.handlers[n].f;
	if (f != NULL) {
		f(n, interrupts_common.handlers[n].data);
	}

	interrupts_clearStatus(n);
	*(interrupts_common.vim + vim_irqvec) = 0; /* Write any value */
}


int hal_interruptsSet(unsigned int n, int (*f)(unsigned int, void *), void *data)
{
	if (n >= SIZE_INTERRUPTS) {
		return -1;
	}

	hal_interruptsDisableAll();
	hal_interruptsDisable(n);

	interrupts_common.handlers[n].data = data;
	interrupts_common.handlers[n].f = f;

	if (f != NULL) {
		interrupts_setPriority(n, DEFAULT_PRIORITY); /* Set the same priority for every IRQ */
		hal_interruptsEnable(n);
	}

	hal_interruptsEnableAll();

	return 0;
}


void interrupts_init(void)
{
	unsigned int i;

	interrupts_common.vim = (void *)VIM_BASE_ADDRESS;

	for (i = 0; i < SIZE_INTERRUPTS; ++i) {
		interrupts_common.handlers[i].data = NULL;
		interrupts_common.handlers[i].f = NULL;
	}

	/* Clear pending and disable interrupts, set them to be handled by IRQ not FIQ */
	for (i = 0; i < (SIZE_INTERRUPTS + 31) / 32; i++) {
		*(interrupts_common.vim + vim_irqsts_m + (i * 8)) = 0xffffffff;
		*(interrupts_common.vim + vim_intr_en_clr_m + (i * 8)) = 0xffffffff;
		*(interrupts_common.vim + vim_intmap_m + (i * 8)) = 0;
	}

	hal_interruptsEnableAll();
}
