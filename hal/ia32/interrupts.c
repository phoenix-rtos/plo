/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupt handling
 *
 * Copyright 2012-2013, 2016-2017, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>


/* Number of hardware interrupts */
#define SIZE_INTERRUPTS 16


typedef struct {
	int (*f)(unsigned int, void *);
	void *data;
} intr_handler_t;


struct {
	intr_handler_t handlers[SIZE_INTERRUPTS];
} interrupts_common;


/* Hardware interrupt stubs */
extern void _interrupts_irq0(void);
extern void _interrupts_irq1(void);
extern void _interrupts_irq2(void);
extern void _interrupts_irq3(void);
extern void _interrupts_irq4(void);
extern void _interrupts_irq5(void);
extern void _interrupts_irq6(void);
extern void _interrupts_irq7(void);
extern void _interrupts_irq8(void);
extern void _interrupts_irq9(void);
extern void _interrupts_irq10(void);
extern void _interrupts_irq11(void);
extern void _interrupts_irq12(void);
extern void _interrupts_irq13(void);
extern void _interrupts_irq14(void);
extern void _interrupts_irq15(void);


/* Unhandled interrupts stub */
extern void _interrupts_unexpected(void);


/* Dispatches interrupt handling */
int interrupts_dispatch(unsigned int n)
{
	if ((n >= SIZE_INTERRUPTS) || (interrupts_common.handlers[n].f == NULL))
		return -EINVAL;

	return interrupts_common.handlers[n].f(n, interrupts_common.handlers[n].data);
}


/* Acknowledges interrupt in controller */
int interrupts_ack(unsigned int n)
{
	if (n >= SIZE_INTERRUPTS)
		return -EINVAL;

	if (n < 8) {
		hal_outb((void *)0x20, 0x60 | n);
	}
	else {
		hal_outb((void *)0x20, 0x62);
		hal_outb((void *)0xa0, 0x60 | (n - 8));
	}

	return EOK;
}


void hal_interruptsDisableAll(void)
{
	__asm__ volatile("cli":);
}


void hal_interruptsEnableAll(void)
{
	__asm__ volatile("sti":);
}


int hal_interruptsSet(unsigned int irq, int (*f)(unsigned int, void *), void *data)
{
	if (irq >= SIZE_INTERRUPTS)
		return -EINVAL;

	hal_interruptsDisableAll();

	interrupts_common.handlers[irq].f = f;
	interrupts_common.handlers[irq].data = data;

	hal_interruptsEnableAll();

	return EOK;
}


/* Sets interrupt stub in IDT */
int interrupts_setIDTEntry(unsigned int n, void (*base)(void), unsigned short sel, unsigned char flags)
{
	struct idt_entry_t {
		u16 low;
		u16 sel;
		u8 res;
		u8 type;
		u16 high;
	} __attribute__((packed)) *entry = (struct idt_entry_t *)ADDR_IDT + n;

	if (n >= 0x100)
		return -EINVAL;

	entry->low = (unsigned int)base;
	entry->high = (unsigned int)base >> 16;
	entry->sel = sel;
	entry->type = flags | 0xe0;
	entry->res = 0;

	return EOK;
}


/* Remaps 8259A PIC vector offsets */
void interrupts_remapPIC(unsigned char offs1, unsigned char offs2)
{
	/* Initialize primary PIC */
	hal_outb((void *)0x20, 0x11);  /* ICW1 - master initialization sequence */
	hal_outb((void *)0x21, offs1); /* ICW2 - master vector offset */
	hal_outb((void *)0x21, 0x04);  /* ICW3 - slave PIC at IRQ2 */
	hal_outb((void *)0x21, 0x01);  /* ICW4 - 8086 mode */

	/* Initialize secondary PIC */
	hal_outb((void *)0xa0, 0x11);  /* ICW1 - slave initialization sequence */
	hal_outb((void *)0xa1, offs2); /* ICW2 - slave vector offset */
	hal_outb((void *)0xa1, 0x02);  /* ICW3 - cascade identity */
	hal_outb((void *)0xa1, 0x01);  /* ICW4 - 8086 mode */
}


void hal_interruptsInit(void)
{
	unsigned int i;

	/* Remap 8259A PIC for protected mode */
	interrupts_remapPIC(0x20, 0x28);

	/* Set hardware interrupt stubs */
	interrupts_setIDTEntry(0x20 + 0, _interrupts_irq0, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 1, _interrupts_irq1, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 2, _interrupts_irq2, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 3, _interrupts_irq3, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 4, _interrupts_irq4, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 5, _interrupts_irq5, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 6, _interrupts_irq6, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 7, _interrupts_irq7, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 8, _interrupts_irq8, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 9, _interrupts_irq9, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 10, _interrupts_irq10, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 11, _interrupts_irq11, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 12, _interrupts_irq12, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 13, _interrupts_irq13, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 14, _interrupts_irq14, 0x08, 0x0e);
	interrupts_setIDTEntry(0x20 + 15, _interrupts_irq15, 0x08, 0x0e);

	/* Set unhandled interrupt stubs */
	for (i = 0x20 + SIZE_INTERRUPTS; i < 0x100; i++)
		interrupts_setIDTEntry(i, _interrupts_unexpected, 0x08, 0x0e);
}
