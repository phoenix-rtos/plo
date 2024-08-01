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

/* Interrupt controller */

#define INT_LEVEL   0  /* Interrupt level register : 0x00 */
#define INT_PEND    1  /* Interrupt pending register : 0x04 */
#define INT_FORCE   2  /* Interrupt force register (CPU 0) : 0x08 */
#define INT_CLEAR   3  /* Interrupt clear register : 0x0C */
#define INT_MPSTAT  4  /* Multiprocessor status register : 0x10 */
#define INT_BRDCAST 5  /* Broadcast register : 0x14 */
#define INT_MASK_0  16 /* Interrupt mask register (CPU 0) : 0x40 */
#define INT_MASK_1  17 /* Interrupt mask register (CPU 1) : 0x44 */
#define INT_FORCE_0 32 /* Interrupt force register (CPU 0) : 0x80 */
#define INT_FORCE_1 33 /* Interrupt force register (CPU 1) : 0x84 */
#define INT_EXTID_0 48 /* Extended interrupt ID register (CPU 0) : 0xC0 */
#define INT_EXTID_1 49 /* Extended interrupt ID register (CPU 1) : 0xC4 */


typedef struct {
	int (*isr)(unsigned int, void *);
	void *data;
} irq_handler_t;


static struct {
	vu32 *int_ctrl;
	irq_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


inline void hal_interruptsEnable(unsigned int irqn)
{
	*(interrupts_common.int_ctrl + INT_MASK_0) |= (1 << irqn);
}


inline void hal_interruptsDisable(unsigned int irqn)
{
	*(interrupts_common.int_ctrl + INT_MASK_0) &= ~(1 << irqn);
}


int interrupts_dispatch(unsigned int irq)
{
	if (irq == EXTENDED_IRQN) {
		/* Extended interrupt (16 - 31) */
		irq = *(interrupts_common.int_ctrl + INT_EXTID_0) & 0x3F;
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
	hal_interruptsEnableAll();
}
