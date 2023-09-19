/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Memory map
 *
 * Copyright 2012, 2020-2022 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "acpi.h"

struct {
	hal_syspage_t *hs;
} memory_common;


/* Linker symbols */
extern char __ramtext_start[], __ramtext_end[];
extern char _start[], _end[];
extern char __data_start[], __data_end[];
extern char __bss_start[], __bss_end[];
extern char __heap_base[], __heap_limit[];
extern char __stack_top[], __stack_limit[];
extern char __init_start[];


void hal_syspageSet(hal_syspage_t *hs)
{
	memory_common.hs = hs;

	memory_common.hs->gdtr.size = SIZE_GDT - 1;
	memory_common.hs->gdtr.addr = ADDR_GDT;

	memory_common.hs->idtr.size = SIZE_IDT - 1;
	memory_common.hs->idtr.addr = ADDR_IDT;

	memory_common.hs->pdir = ADDR_PDIR;
	memory_common.hs->ptable = ADDR_PTABLE;
	memory_common.hs->stack = (addr_t)__stack_top;

	memory_common.hs->stacksz = __stack_top - __stack_limit;
	hal_acpiInit(hs);
}


addr_t hal_memoryGetSyspageAddr(void)
{
	return (addr_t)memory_common.hs;
}


int hal_memoryGetEntry(unsigned int *offs, mapent_t *entry, unsigned int *biosType)
{
	struct {
		unsigned long long addr;
		unsigned long long len;
		unsigned int type;
	} mem;

	int ret = ((unsigned int)&mem & 0xffff0000) >> 4;
	unsigned int next = *offs;

	hal_memset(entry, 0, sizeof(mapent_t));

	__asm__ volatile(
		"pushl $0x15; "
		"pushl $0x0; "
		"pushl %%eax; "
		"movl $0x534d4150, %%edx; "
		"movl $0x14, %%ecx; "
		"movl $0xe820, %%eax; "
		"call _interrupts_bios; "
		"jc 0f; "
		"xorl %%eax, %%eax; "
		"jmp 1f; "
		"0: "
		"movl $0x1, %%eax; "
		"1: "
		"addl $0xc, %%esp; "
	: "+a" (ret), "+b" (next)
	: "D" (&mem)
	: "ecx", "edx", "memory", "cc");

	*offs = next;

	entry->start = mem.addr;
	/* TODO: mapent_t should use addr & len instead of start & end */
	entry->end = (mem.addr + mem.len <= 0xffffffff) ? mem.len + mem.addr : mem.addr + mem.len - 1;
	entry->type = hal_entryReserved;

	*biosType = mem.type;

	return ret;
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
	unsigned int i, offs, biosType;
	mapent_t tempEntry, prevEntry, minEntry;

	/* The following entries are used only by plo - type = hal_entryTemp, kernel defines them by its own */
	static const mapent_t entries[] = {
		{ .start = 0x0, .end = 0x1000, .type = hal_entryTemp },
		{ .start = (addr_t)_start, .end = (addr_t)_end, .type = hal_entryTemp },
		{ .start = (addr_t)__ramtext_start, .end = (addr_t)__ramtext_end, .type = hal_entryTemp },
		{ .start = (addr_t)__data_start, .end = (addr_t)__data_end, .type = hal_entryTemp },
		{ .start = (addr_t)__bss_start, .end = (addr_t)__bss_end, .type = hal_entryTemp },
		{ .start = (addr_t)__heap_base, .end = (addr_t)__heap_limit, .type = hal_entryTemp },
		{ .start = (addr_t)__stack_limit, .end = (addr_t)__stack_top, .type = hal_entryTemp },
		{ .start = ADDR_GDT, .end = ADDR_GDT + SIZE_GDT, .type = hal_entryTemp },
		{ .start = ADDR_IDT, .end = ADDR_IDT + SIZE_IDT, .type = hal_entryTemp },
		/* TODO: this entry should be removed after changes in disk-bios */
		{ .start = ADDR_RCACHE, .end = ADDR_RCACHE + SIZE_RCACHE + SIZE_WCACHE, .type = hal_entryTemp },
	};

	if (start == end) {
		return -1;
	}

	hal_memset(&tempEntry, 0, sizeof(tempEntry));
	hal_memset(&prevEntry, 0, sizeof(prevEntry));
	hal_memset(&minEntry, 0, sizeof(minEntry));
	minEntry.start = (addr_t)-1;

	/* Syspage entry */
	tempEntry.start = (addr_t)memory_common.hs;
	tempEntry.end = (addr_t)__heap_limit;
	tempEntry.type = hal_entryTemp;
	hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);

	for (i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i) {
		if (entries[i].start >= entries[i].end) {
			continue;
		}
		hal_memcpy(&tempEntry, &entries[i], sizeof(mapent_t));
		hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);
	}

	/* Get BIOS e820 memory map */
	offs = 0;
	while (1) {
		if (hal_memoryGetEntry(&offs, &tempEntry, &biosType) != 0) {
			return -1;
		}

		/* BIOS areas of different type than FREE are added to map's entries list */
		if (biosType != 0x1) {
			hal_getMinOverlappedRange(start, end, &tempEntry, &minEntry);
		}

		/* The gap between BIOS entries is assigned as invalid entry */
		if (prevEntry.end != tempEntry.start) {
			prevEntry.start = prevEntry.end;
			prevEntry.end = tempEntry.start;
			prevEntry.type = hal_entryInvalid;
			hal_getMinOverlappedRange(start, end, &prevEntry, &minEntry);
		}

		hal_memcpy(&prevEntry, &tempEntry, sizeof(mapent_t));

		if (offs == 0) {
			break;
		}
	}

	if (minEntry.start != (addr_t)-1) {
		entry->start = minEntry.start;
		entry->end = minEntry.end;
		entry->type = minEntry.type;

		return 0;
	}
	/* Get ACPI memory map */

	return -1;
}


/* Flushes 8042 keyboard controller buffers */
static int memory_empty8042(void)
{
	unsigned char status;
	unsigned short int timeout;
	for (timeout = 0xffff; timeout != 0; --timeout) {
		/* Discard input data */
		status = hal_inb((void *)0x64);
		if ((status & 0x01) != 0) {
			hal_inb((void *)0x60);
			continue;
		}
		if ((status & 0x02) == 0) {
			return 0;
		}
	}
	return -1;
}


static int checkA20Line(void)
{
	int result = 0;
	/* Set base to some address in the first 1MiB that can be overwritten */
	/* Address of the magic number at the end of MBR (0xAA55) */
	volatile u16 *const base = (void *)(__init_start + 512 - 2);
	/* Some values that should be equal, if A20 is off */
	u16 v1 = *(base + (0x100000 / sizeof(*base)));
	u16 v2 = *base;
	if (v1 == v2) {
		/* They are equal: try to overwrite one of them */
		*base = v2 + 1;
		/* Did value that shouldn't change, changed? */
		if ((*(base + (0x100000 / sizeof(*base)))) != v1) {
			/* A20 is off */
			result = -1;
		}
		*base = v2;
	}
	return result;
}


/* Enables A20 line mask */
static int memory_enableA20(void)
{
	if (checkA20Line() < 0) {
		if (memory_empty8042() < 0) {
			return -1;
		}
		hal_outb((void *)0x64, 0xd1);
		if (memory_empty8042() < 0) {
			return -1;
		}
		hal_outb((void *)0x60, 0xdf);
		if (memory_empty8042() < 0) {
			return -1;
		}
	}
	return 0;
}


int hal_memoryAddMap(addr_t start, addr_t end, u32 attr, u32 mapId)
{
	return 0;
}


int hal_memoryInit(void)
{
	if (memory_enableA20() < 0) {
		hal_consolePrint("hal: Failed to enable A20 line\n");
		return -1;
	}

	return 0;
}
