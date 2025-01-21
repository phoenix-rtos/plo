/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupt handling - IRQAMP controller
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

/* Hardware interrupts */

#define SIZE_INTERRUPTS 32

/* clang-format off */

/* Interrupt controller
 * NOTE: Some registers may not be available depending on the configuration
 */
enum {
	int_level = 0,     /* Interrupt level register                   : 0x000 */
	int_pend,          /* Interrupt pending register                 : 0x004 */
	int_force,         /* Interrupt force register (CPU 0)           : 0x008 */
	int_clear,         /* Interrupt clear register                   : 0x00C */
	int_mpstat,        /* Status register                            : 0x010 */
	broadcast,         /* Broadcast register                         : 0x014 */
	errstat,           /* Error mode status register                 : 0x018 */
	wdogctrl,          /* Watchdog control register                  : 0x01C */
	asmpctrl,          /* Asymmetric MP control register             : 0x020 */
	icselr,            /* Interrupt controller select register       : 0x024 */
	                   /* Reserved                                   : 0x028 - 0x030 */
	eint_clear = 13,   /* Extended interrupt clear register          : 0x034 */
	                   /* Reserved                                   : 0x038 - 0x03C */
	pi_mask = 16,      /* Processor interrupt mask register (CPU 0)  : 0x040 */
	                   /* NUM_CPUS pi_mask registers                         */
	pc_force = 32,     /* Processor interrupt force register (CPU 0) : 0x080 */
	                   /* NUM_CPUS pc_force registers                        */
	pextack = 48,      /* Extended int acknowledge register (CPU 0)  : 0x0C0 */
	                   /* NUM_CPUS pextack registers                         */
	tcnt0 = 64,        /* Interrupt timestamp 0 counter register     : 0x100 */
	istmpc0,           /* Timestamp 0 control register               : 0x104 */
	itstmpas0,         /* Interrupt assertion timestamp 0 register   : 0x108 */
	itstmpack0,        /* Interrupt acknowledge timestamp 0 register : 0x10C */
	tcnt1,             /* Interrupt timestamp 1 counter register     : 0x110 */
	istmpc1,           /* Timestamp 1 control register               : 0x114 */
	itstmpas1,         /* Interrupt assertion timestamp 1 register   : 0x118 */
	itstmpack1,        /* Interrupt acknowledge timestamp 1 register : 0x11C */
	tcnt2,             /* Interrupt timestamp 2 counter register     : 0x120 */
	istmpc2,           /* Timestamp 2 control register               : 0x124 */
	itstmpas2,         /* Interrupt assertion timestamp 2 register   : 0x128 */
	itstmpack2,        /* Interrupt acknowledge timestamp 2 register : 0x12C */
	tcnt3,             /* Interrupt timestamp 3 counter register     : 0x130 */
	istmpc3,           /* Timestamp 3 control register               : 0x134 */
	itstmpas3,         /* Interrupt assertion timestamp 3 register   : 0x138 */
	itstmpack3,        /* Interrupt acknowledge timestamp 3 register : 0x13C */
	                   /* Reserved                                   : 0x140 - 0x1FC */
	procbootadr = 128, /* Processor boot address register (CPU 0)    : 0x200 */
	                   /* NUM_CPUS procbootadr registers                     */
	irqmap = 192,      /* Interrupt map register                     : 0x300 - impl dependent no. entries */
};

/* clang-format on */

typedef struct {
	int (*isr)(unsigned int, void *);
	void *data;
} irq_handler_t;


static struct {
	vu32 *int_ctrl;
	u32 extendedIrqn;
	irq_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


inline void hal_interruptsEnable(unsigned int irqn)
{
	*(interrupts_common.int_ctrl + pi_mask) |= (1 << irqn);
}


inline void hal_interruptsDisable(unsigned int irqn)
{
	*(interrupts_common.int_ctrl + pi_mask) &= ~(1 << irqn);
}


__attribute__((section(".noxip"))) int interrupts_dispatch(unsigned int irq)
{
	if (irq == interrupts_common.extendedIrqn) {
		/* Extended interrupt (16 - 31) */
		irq = *(interrupts_common.int_ctrl + pextack) & 0x3F;
	}

	if ((irq >= SIZE_INTERRUPTS) || (interrupts_common.handlers[irq].isr == NULL)) {
		return -1;
	}

	interrupts_common.handlers[irq].isr(irq, interrupts_common.handlers[irq].data);

	return 0;
}


int hal_interruptsSet(unsigned int n, int (*isr)(unsigned int, void *), void *data)
{
	if (n >= SIZE_INTERRUPTS || n == 0) {
		return -1;
	}

	hal_interruptsDisableAll();
	interrupts_common.handlers[n].data = data;
	interrupts_common.handlers[n].isr = isr;

	if (isr == NULL) {
		hal_interruptsDisable(n);
	}
	else {
		hal_interruptsEnable(n);
	}

	hal_interruptsEnableAll();

	return 0;
}


void interrupts_init(void)
{
	interrupts_common.int_ctrl = (u32 *)INT_CTRL_BASE;

	/* Read extended irqn */
	interrupts_common.extendedIrqn = (*(interrupts_common.int_ctrl + int_mpstat) >> 16) & 0xf;

	hal_interruptsEnableAll();
}
