/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupt handling - NVIC (Nested Vectored Interrupt Controller)
 *
 * Copyright 2017, 2019, 2020, 2022 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


typedef struct {
	void *data;
	int (*isr)(unsigned int, void *);
} intr_handler_t;


/* clang-format off */
enum { nvic_iser = 0, nvic_icer = 32, nvic_ispr = 64, nvic_icpr = 96, nvic_iabr = 128,
	nvic_ip = 192 };
/* clang-format on */


struct {
	intr_handler_t irqs[SIZE_INTERRUPTS];
	volatile u32 *nvic;
} irq_common;


static void interrupts_nvicSetIRQ(u8 irqn, u8 state)
{
	volatile u32 *ptr = irq_common.nvic + (irqn >> 5) + (state ? nvic_iser : nvic_icer);
	*ptr = 1u << (irqn & 0x1fu);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


static void interrupts_nvicSetPriority(u8 irqn, u32 priority)
{
	volatile u32 *ptr;

	ptr = ((u32 *)(irq_common.nvic + nvic_ip)) + (irqn / 4);

	*ptr = (priority << (8 * (irqn % 4)));
}


void hal_interruptsEnableAll(void)
{
	__asm__ volatile("cpsie if");
}


void hal_interruptsDisableAll(void)
{
	__asm__ volatile("cpsid if");
}


void hal_interruptsEnable(unsigned int irqn)
{
	if (irqn >= SIZE_INTERRUPTS || irqn < 16) {
		return;
	}

	interrupts_nvicSetIRQ(irqn - 16, 1);
}


void hal_interruptsDisable(unsigned int irqn)
{
	if (irqn >= SIZE_INTERRUPTS || irqn < 16) {
		return;
	}

	interrupts_nvicSetIRQ(irqn - 16, 0);
}


int hal_interruptsSet(unsigned int irq, int (*isr)(unsigned int, void *), void *data)
{
	if (irq >= SIZE_INTERRUPTS) {
		return -1;
	}

	hal_interruptsDisableAll();
	irq_common.irqs[irq].isr = isr;
	irq_common.irqs[irq].data = data;

	/* Is it a IRQ handled by NVIC? */
	if (irq >= 16) {
		/* when isr is set to NULL - disables the interrupt */
		if (isr == NULL) {
			interrupts_nvicSetIRQ(irq - 16, 0);
		}
		else {
			interrupts_nvicSetPriority(irq - 16, 0);
			interrupts_nvicSetIRQ(irq - 16, 1);
		}
	}
	hal_interruptsEnableAll();

	return 0;
}


int hal_interruptDispatch(u32 exceptionNr)
{
	/* on Phoenix-RTOS we treat exception numbers as irqs */
	u32 irq = exceptionNr;
	if (irq_common.irqs[irq].isr == NULL) {
		return -1;
	}

	irq_common.irqs[irq].isr(irq, irq_common.irqs[irq].data);

	return 0;
}


void interrupts_init(void)
{
	/* nvic_iser0 register address */
	irq_common.nvic = (void *)0xe000e100u;
}
