/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2021-2025 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#include "../cache.h"


struct {
	/* These fields are used in assembly code in _init.S, don't reorder them */
	hal_syspage_t *hs;
	addr_t entry;
} hal_common;


/* Linker symbols */
extern char __init_start[], __init_end[];
extern char __text_start[], __etext[];
extern char __rodata_start[], __rodata_end[];
extern char __cmd_start[], __cmd_end[];
extern char __init_array_start[], __init_array_end[];
extern char __fini_array_start[], __fini_array_end[];
extern char __ramtext_start[], __ramtext_end[];
extern char __data_start[], __data_end[];
extern char __bss_start[], __bss_end[];
extern char __heap_base[], __heap_limit[];
extern char __stack_top[], __stack_limit[];


/* Timer */
extern void timer_init(void);
extern void timer_done(void);

/* Interrupts */
extern void interrupts_init(void);

/* Console */
void console_init(void);

extern unsigned int hal_getBootReason(void);


void hal_init(void)
{
	_tda4vm_init();
	interrupts_init();

	mpu_init();
	timer_init();
	console_init();

	hal_common.entry = (addr_t)-1;
}


void hal_done(void)
{
	timer_done();
}


void hal_syspageSet(hal_syspage_t *hs)
{
	hal_common.hs = hs;
	/* TODO: determine reset reason */
	hal_common.hs->resetReason = 0;
}


const char *hal_cpuInfo(void)
{
	/* TODO: for some reason the last character of this name is cut off when printed */
	return "Cortex-R5F TI K3 J721E SoC ";
}


addr_t hal_kernelGetAddress(addr_t addr)
{
	return addr;
}


void hal_kernelGetEntryPointOffset(addr_t *off, int *indirect)
{
	*off = 0;
	*indirect = 1;
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
		if (start > entry->start) {
			entry->start = start;
		}

		if (end < entry->end) {
			entry->end = end;
		}

		if (entry->start < minEntry->start) {
			minEntry->start = entry->start;
			minEntry->end = entry->end;
			minEntry->type = entry->type;
		}
	}
}


int hal_memoryGetNextEntry(addr_t start, addr_t end, mapent_t *entry)
{
	int i;
	mapent_t tempEntry, minEntry;

	static const mapent_t entries[] = {
		{ .start = (addr_t)__init_start, .end = (addr_t)__init_end, .type = hal_entryTemp },
		{ .start = (addr_t)__text_start, .end = (addr_t)__etext, .type = hal_entryTemp },
		{ .start = (addr_t)__rodata_start, .end = (addr_t)__rodata_end, .type = hal_entryTemp },
		{ .start = (addr_t)__cmd_start, .end = (addr_t)__cmd_end, .type = hal_entryTemp },
		{ .start = (addr_t)__init_array_start, .end = (addr_t)__init_array_end, .type = hal_entryTemp },
		{ .start = (addr_t)__fini_array_start, .end = (addr_t)__fini_array_end, .type = hal_entryTemp },
		{ .start = (addr_t)__ramtext_start, .end = (addr_t)__ramtext_end, .type = hal_entryTemp },
		{ .start = (addr_t)__data_start, .end = (addr_t)__data_end, .type = hal_entryTemp },
		{ .start = (addr_t)__bss_start, .end = (addr_t)__bss_end, .type = hal_entryTemp },
		{ .start = (addr_t)__heap_base, .end = (addr_t)__heap_limit, .type = hal_entryTemp },
		{ .start = (addr_t)__stack_limit, .end = (addr_t)__stack_top, .type = hal_entryTemp },
	};

	if (start == end) {
		return -1;
	}

	minEntry.start = (addr_t)-1;
	minEntry.end = 0;
	minEntry.type = 0;

	/* Syspage entry */
	tempEntry.start = (addr_t)hal_common.hs;
	tempEntry.end = (addr_t)__heap_limit;
	tempEntry.type = hal_entryReserved;
	hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);

	for (i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
		if (entries[i].start >= entries[i].end) {
			continue;
		}
		tempEntry.start = entries[i].start;
		tempEntry.end = entries[i].end;
		tempEntry.type = entries[i].type;
		hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);
	}

	if (minEntry.start != (addr_t)-1) {
		entry->start = minEntry.start;
		entry->end = minEntry.end;
		entry->type = minEntry.type;

		return 0;
	}

	return -1;
}


void hal_cpuReboot(void)
{
	tda4vm_warmReset();

	__builtin_unreachable();
}


int hal_cpuJump(void)
{
	if (hal_common.entry == (addr_t)-1) {
		return -1;
	}

	mpu_getHalData(hal_common.hs);
	hal_interruptsDisableAll();

	hal_dcacheEnable(0);
	/* Flushing cache not necessary - memory is configured in _init.S to be write-through */
	hal_icacheEnable(0);

	/* Removing region that remaps our interrupt vectors.
	 * Interrupts MUST be disabled before this is done! */
	tda4vm_RATUnmapMemory(RAT_REGION_VECTORS_REMAP);

	/* clang-format off */
	__asm__ volatile(
		"mov r9, %1\n\t"
		"bx %0"
		:
		: "r"(hal_common.entry), "r"((addr_t)hal_common.hs));
	/* clang-format on */
	__builtin_unreachable();
}
