/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski, Marcin Baran
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <syspage.h>
#include <hal/hal.h>
#include <lib/errno.h>

typedef struct {
	void *data;
	int (*isr)(u16, void *);
} intr_handler_t;


struct{
	addr_t kernel_entry;
	intr_handler_t irqs[SIZE_INTERRUPTS];
} hal_common;


/* Linker symbols */
extern void _end(void);
extern void _plo_bss(void);

/* Timer */
extern void timer_init(void);
extern void timer_done(void);

/* Console */
extern void console_init(void);


void hal_init(void)
{
	_imxrt_init();
	timer_init();

	hal_consoleInit();

	syspage_init();
	syspage_setAddress((void *)SYSPAGE_ADDRESS);

	/* Add entries related to plo image */
	syspage_addEntries((u32)_plo_bss, (u32)_end - (u32)_plo_bss + STACK_SIZE);
}


void hal_done(void)
{
	timer_done();

	_imxrt_cleanDCache();
}


const char *hal_cpuInfo(void)
{
	return "Cortex-M i.MX RT117x";
}


void hal_setKernelEntry(addr_t addr)
{
	hal_common.kernel_entry = addr;
}


addr_t hal_vm2phym(addr_t addr)
{
	return addr;
}


int hal_launch(void)
{
	time_t start;

	syspage_save();

	/* Give the LPUART transmitters some time */
	start = hal_getTime();
	while ((hal_getTime() - start) < 100);

	/* Tidy up */
	hal_done();

	hal_cli();
	__asm__ volatile("mov r9, %1; \
		 blx %0"
		 :
		 : "r"(hal_common.kernel_entry), "r"(syspage_getAddress()));
	hal_sti();

	return -1;
}


extern void hal_invalDCacheAll(void)
{
	/* TODO */
}


void hal_invalDCacheAddr(addr_t addr, size_t sz)
{
	/* TODO */
}


void hal_cli(void)
{
	__asm__ volatile("cpsid if");
}


void hal_sti(void)
{
	__asm__ volatile("cpsie if");
}


int hal_irqdispatch(u16 irq)
{
	if (hal_common.irqs[irq].isr == NULL)
		return -1;

	hal_common.irqs[irq].isr(irq, hal_common.irqs[irq].data);

	return 0;
}


int hal_irqinst(u16 irq, int (*isr)(u16, void *), void *data)
{
	if (irq >= SIZE_INTERRUPTS)
		return -EINVAL;

	hal_cli();
	hal_common.irqs[irq].isr = isr;
	hal_common.irqs[irq].data = data;

	_imxrt_nvicSetPriority(irq - 0x10, 1);
	_imxrt_nvicSetIRQ(irq - 0x10, 1);
	hal_sti();

	return 0;
}


int hal_irquninst(u16 irq)
{
	hal_cli();
	_imxrt_nvicSetIRQ(irq - 0x10, 0);
	hal_sti();

	return 0;
}
