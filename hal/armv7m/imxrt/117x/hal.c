/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2020-2021 Phoenix Systems
 * Author: Hubert Buczynski, Marcin Baran, Gerard Swiderski
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


struct{
	intr_handler_t irqs[SIZE_INTERRUPTS];
	hal_syspage_t *hs;

	addr_t entry;
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

	mpu_init();
	timer_init();
	console_init();

	hal_common.entry = (addr_t)-1;
}


void hal_done(void)
{
	timer_done();

	_imxrt_cleanDCache();
}


void hal_syspageSet(hal_syspage_t *hs)
{
	hal_common.hs = hs;
}


const char *hal_cpuInfo(void)
{
	return "Cortex-M i.MX RT117x";
}


void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz)
{
	switch (type) {
		case hal_cpuDCache:
			/* TODO */
		case hal_cpuICache:
			/* TODO */
		default:
			break;
	}
}


addr_t hal_kernelGetAddress(addr_t addr)
{
	return addr;
}


void hal_kernelEntryPoint(addr_t addr)
{
	hal_common.entry = addr;
}


int hal_memoryAddMap(addr_t start, addr_t end, u32 attr, u32 mapId)
{
	return mpu_regionAlloc(start, end, attr, mapId, 1);
}


static void hal_getMinOverlappedRange(addr_t start, addr_t end, mapent_t *entry, mapent_t *minEntry)
{
	if ((start < entry->end) && (end > entry->start)) {
		if (start > entry->start)
			entry->start = start;

		if (end < entry->end)
			entry->end = end;

		if (entry->start < minEntry->start) {
			minEntry->start = entry->start;
			minEntry->end = entry->end;
			minEntry->type = entry->type;
		}
	}
}


int hal_memoryGetNextEntry(addr_t start, addr_t end, mapent_t *entry)
{
	mapent_t tempEntry = { 0 };
	mapent_t minEntry = { .start = (addr_t)-1, .end = 0 };

	if (start == end)
		return -1;

	/* plo: .bss */
	tempEntry.start = (addr_t)_plo_bss;
	tempEntry.end = (addr_t)_end;
	tempEntry.type = hal_entryTemp;
	hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);

	/* syspage */
	tempEntry.start = (addr_t)hal_common.hs;
	tempEntry.end = tempEntry.start + SIZE_SYSPAGE;
	tempEntry.type = hal_entryReserved;
	hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);

	/* plo: .stack */
	tempEntry.start = (addr_t)ADDR_STACK;
	tempEntry.end = tempEntry.start + SIZE_STACK;
	tempEntry.type = hal_entryTemp;
	hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);

	if (minEntry.start != (addr_t)-1) {
		entry->start = minEntry.start;
		entry->end = minEntry.end;
		entry->type = minEntry.type;

		return 0;
	}

	return -1;
}


int hal_cpuJump(void)
{
	if (hal_common.entry == (addr_t)-1)
		return -1;

	hal_interruptsDisable();

	mpu_getHalData(hal_common.hs);

	__asm__ volatile("mov r9, %1; \
		 blx %0"
		:
		: "r"(hal_common.entry), "r"((addr_t)hal_common.hs));

	return 0;
}


void hal_interruptsDisable(void)
{
	__asm__ volatile("cpsid if");
}


void hal_interruptsEnable(void)
{
	__asm__ volatile("cpsie if");
}


int hal_interruptsSet(unsigned int irq, int (*isr)(unsigned int, void *), void *data)
{
	if (irq >= SIZE_INTERRUPTS)
		return -1;

	hal_interruptsDisable();
	hal_common.irqs[irq].isr = isr;
	hal_common.irqs[irq].data = data;

	if (isr == NULL) {
		_imxrt_nvicSetIRQ(irq - 0x10, 0);
	}
	else {
		_imxrt_nvicSetPriority(irq - 0x10, 1);
		_imxrt_nvicSetIRQ(irq - 0x10, 1);
	}
	hal_interruptsEnable();

	return 0;
}


int hal_irqdispatch(unsigned int irq)
{
	if (hal_common.irqs[irq].isr == NULL)
		return -1;

	hal_common.irqs[irq].isr(irq, hal_common.irqs[irq].data);

	return 0;
}
