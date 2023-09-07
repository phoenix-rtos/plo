/*
 * Phoenix-RTOS
 *
 * Operating system loader
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

#include "dtb.h"

#include <hal/hal.h>
#include <board_config.h>


#define STR(x)  #x
#define XSTR(x) STR(x)

#define CLINT_IRQ_SIZE 16

#define EXT_IRQ 9

typedef struct {
	int (*isr)(unsigned int, void *);
	void *data;
} irq_handler_t;


static struct {
	vu32 *intCtrl;
	irq_handler_t clintHandlers[CLINT_IRQ_SIZE];
	irq_handler_t plicHandlers[PLIC_IRQ_SIZE];
} interrupts_common;


void hal_interruptsEnable(unsigned int irqn)
{
	if ((irqn & CLINT_IRQ_FLG) != 0) {
		csr_set(sie, 1u << (irqn & ~CLINT_IRQ_FLG));
	}
	else {
		csr_set(sie, 1u << EXT_IRQ);
		plic_enableInterrupt(1, irqn);
	}
}


void hal_interruptsDisable(unsigned int irqn)
{
	if ((irqn & CLINT_IRQ_FLG) != 0) {
		csr_clear(sie, 1u << (irqn & ~CLINT_IRQ_FLG));
	}
	else {
		plic_disableInterrupt(1, irqn);
	}
}


void hal_interruptsEnableAll(void)
{
	csr_set(sstatus, SSTATUS_SIE);
}


void hal_interruptsDisableAll(void)
{
	csr_clear(sstatus, SSTATUS_SIE);
}


static void interrupts_dispatchPlic(void)
{
	unsigned int irq = plic_claim(1);

	if (irq == 0) {
		return;
	}

	if ((irq < PLIC_IRQ_SIZE) && (interrupts_common.plicHandlers[irq].isr != NULL)) {
		interrupts_common.plicHandlers[irq].isr(irq, interrupts_common.plicHandlers[irq].data);
	}

	plic_complete(1, irq);
}


static void interrupts_dispatchClint(unsigned int irq)
{
	if ((irq < CLINT_IRQ_SIZE) && (interrupts_common.clintHandlers[irq].isr != NULL)) {
		interrupts_common.clintHandlers[irq].isr(irq, interrupts_common.clintHandlers[irq].data);
	}
}


void interrupts_dispatch(unsigned int irq)
{
	if ((irq == EXT_IRQ) && (dtb_getPLIC() != 0)) {
		interrupts_dispatchPlic();
	}
	else {
		interrupts_dispatchClint(irq);
	}
}


static int interrupts_setPlic(unsigned int n, int (*isr)(unsigned int, void *), void *data)
{
	if ((n >= PLIC_IRQ_SIZE) || (dtb_getPLIC() == 0)) {
		return -1;
	}

	interrupts_common.plicHandlers[n].data = data;
	interrupts_common.plicHandlers[n].isr = isr;

	if (isr == NULL) {
		plic_disableInterrupt(1, n);
	}
	else {
		plic_priority(n, 2);
		plic_enableInterrupt(1, n);
	}

	return 0;
}


static int interrupts_setClint(unsigned int n, int (*isr)(unsigned int, void *), void *data)
{
	if (n >= CLINT_IRQ_SIZE) {
		return -1;
	}

	interrupts_common.clintHandlers[n].data = data;
	interrupts_common.clintHandlers[n].isr = isr;

	if (isr == NULL) {
		csr_clear(sie, 1u << n);
	}
	else {
		csr_set(sie, 1u << n);
	}

	return 0;
}


int hal_interruptsSet(unsigned int n, int (*isr)(unsigned int, void *), void *data)
{
	int ret;

	hal_interruptsDisableAll();

	if ((n & CLINT_IRQ_FLG) != 0) {
		ret = interrupts_setClint(n & ~CLINT_IRQ_FLG, isr, data);
	}
	else {
		ret = interrupts_setPlic(n, isr, data);
	}

	hal_interruptsEnableAll();

	return ret;
}


extern void _interrupts_dispatch(void);


void interrupts_init(void)
{
	size_t i;
	for (i = 0; i < CLINT_IRQ_SIZE; ++i) {
		interrupts_common.clintHandlers[i].data = NULL;
		interrupts_common.clintHandlers[i].isr = NULL;
	}

	for (i = 0; i < PLIC_IRQ_SIZE; ++i) {
		interrupts_common.plicHandlers[i].data = NULL;
		interrupts_common.plicHandlers[i].isr = NULL;
	}

	csr_write(sscratch, 0);
	csr_write(sie, -1);
	csr_write(stvec, _interrupts_dispatch);

	if (dtb_getPLIC() != 0) {
		_plic_init();
	}
}
