/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2022-2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


#define ASI_FLUSH_DCACHE 0x11


static struct {
	hal_syspage_t *hs;
	addr_t entry;
} hal_common;


/* Linker symbols */
extern char __init_start[], __init_end[];
extern char __text_start[], __etext[];
extern char __rodata_start[], __rodata_end[];
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


void hal_init(void)
{
	_gr712rc_init();
	interrupts_init();

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
}


const char *hal_cpuInfo(void)
{
	return "LEON3FT GR712RC";
}


addr_t hal_kernelGetAddress(addr_t addr)
{
	return addr - VADDR_KERNEL_INIT + ADDR_SRAM;
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
	return 0;
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


void hal_cpuFlushDCache(void)
{
	/* clang-format off */
	__asm__ volatile(
		"sta %%g0, [%%g0] %c0\n\t"
		:
		: "i"(ASI_FLUSH_DCACHE)
	);
	/* clang-format on */
}


void hal_cpuFlushICache(void)
{
	__asm__ volatile("flush");
}


int hal_cpuJump(void)
{
	if (hal_common.entry == (addr_t)-1) {
		return -1;
	}
	hal_interruptsDisableAll();

	__asm__ volatile(
		"jmp %0;"
		"mov %1, %%g2;"
		:
		: "r"(hal_common.entry), "r"(hal_common.hs)
		:);

	return 0;
}


void hal_cpuReboot(void)
{
	/* TODO */
	for (;;) {
	}

	__builtin_unreachable();
}
