/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <devices/gpio-zynq7000/gpio.h>

#include "../mmu.h"
#include "../cache.h"


struct {
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
extern char __ddr_start[], __ddr_end[];
extern char __uncached_ddr_start[], __uncached_ddr_end[];


/* Timer */
extern void timer_init(void);
extern void timer_done(void);

/* Interrupts */
extern void interrupts_init(void);

/* Console */
void console_init(void);


static void hal_memoryInit(void)
{
	int sz = 0;
	addr_t addr;

	mmu_init();

	/* Define on-chip RAM memory as cached */
	for (sz = 0; sz <= SIZE_OCRAM_LOW; sz += SIZE_MMU_SECTION_REGION) {
		addr = ADDR_OCRAM_LOW + sz;
		mmu_mapAddr(addr, addr, MMU_FLAG_CACHED);
	}

	for (sz = 0; sz < SIZE_OCRAM_HIGH; sz += SIZE_MMU_SECTION_REGION) {
		addr = (ADDR_OCRAM_HIGH & ~(SIZE_MMU_SECTION_REGION - 1)) + sz;
		mmu_mapAddr(addr, addr, MMU_FLAG_CACHED);
	}

	/* Define DDR memory as cached */
	for (sz = 0; sz < SIZE_DDR; sz += SIZE_MMU_SECTION_REGION) {
		addr = ADDR_DDR + sz;
		mmu_mapAddr(addr, addr, MMU_FLAG_CACHED);
	}

	for (sz = 0; sz < SIZE_BITSTREAM; sz += SIZE_MMU_SECTION_REGION) {
		addr = ADDR_BITSTREAM + sz;
		mmu_mapAddr(addr, addr, MMU_FLAG_UNCACHED | MMU_FLAG_XN);
	}

	/* Define uncached DDR memory */
	for (sz = 0; sz < SIZE_UNCACHED_BUFF_DDR; sz += SIZE_MMU_SECTION_REGION) {
		addr = ADDR_UNCACHED_BUFF_DDR + sz;
		mmu_mapAddr(addr, addr, MMU_FLAG_UNCACHED | MMU_FLAG_XN);
	}

	mmu_enable();
}


void hal_init(void)
{
	_zynq_init();
	hal_memoryInit();
	interrupts_init();

	gpio_init();
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
	return "Cortex-A9 Zynq 7000";
}


addr_t hal_kernelGetAddress(addr_t addr)
{
	addr_t offs;

	if ((addr_t)VADDR_KERNEL_INIT != (addr_t)ADDR_DDR) {
		offs = addr - VADDR_KERNEL_INIT;
		addr = ADDR_DDR + offs;
	}

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
	return 0;
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
		{ .start = (addr_t)__ddr_start, .end = (addr_t)__ddr_end, .type = hal_entryTemp },
		{ .start = (addr_t)__uncached_ddr_start, .end = (addr_t)__uncached_ddr_end, .type = hal_entryTemp },
		{ .start = (addr_t)ADDR_BITSTREAM, .end = (addr_t)SIZE_BITSTREAM, .type = hal_entryTemp },
	};

	if (start == end)
		return -1;

	minEntry.start = (addr_t)-1;
	minEntry.end = 0;
	minEntry.type = 0;

	/* Syspage entry */
	tempEntry.start = (addr_t)hal_common.hs;
	tempEntry.end = (addr_t)__heap_limit;
	tempEntry.type = hal_entryReserved;
	hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);

	for (i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
		if (entries[i].start >= entries[i].end)
			continue;
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
	_zynq_softRst();

	__builtin_unreachable();
}


int hal_cpuJump(void)
{
	if (hal_common.entry == (addr_t)-1)
		return -1;

	hal_interruptsDisableAll();

	hal_dcacheEnable(0);
	hal_dcacheFlush((addr_t)ADDR_OCRAM_LOW, (addr_t)ADDR_OCRAM_LOW + SIZE_OCRAM_LOW);
	hal_dcacheFlush((addr_t)ADDR_OCRAM_HIGH, (addr_t)ADDR_OCRAM_HIGH + SIZE_OCRAM_HIGH);
	hal_dcacheFlush((addr_t)ADDR_DDR, (addr_t)ADDR_DDR + SIZE_DDR);

	hal_icacheEnable(0);
	hal_icacheInval();

	mmu_disable();

	__asm__ volatile("mov r9, %1; \
		 blx %0"
		:
		: "r"(hal_common.entry), "r"((addr_t)hal_common.hs));

	return 0;
}
