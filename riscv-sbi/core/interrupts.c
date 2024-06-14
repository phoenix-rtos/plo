/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * Interrupt handling
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "csr.h"
#include "devices/clint.h"


void interrupts_dispatch(unsigned int irq)
{
	switch (irq) {
		case IRQ_M_SOFT:
			/* TODO: handle IPI */
			break;

		case IRQ_M_TIMER:
			clint_timerIrqHandler();
			break;

		default:
			break;
	}
}
