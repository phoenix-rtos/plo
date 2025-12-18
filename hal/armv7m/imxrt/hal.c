/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2020-2022 Phoenix Systems
 * Author: Hubert Buczynski, Marcin Baran, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <board_config.h>
#include <hal/hal.h>
#include <devices/devs.h>
#include <lib/helpers.h>
#include <lib/console.h>
#include "otp.h"


struct {
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
#if RTT_ENABLED_PLO
extern char __rttmem_rttcb[];
#endif

/* Timer */
extern void timer_init(void);
extern void timer_done(void);

/* RTT pipes */
extern void rtt_init(void *addr);

/* Console */
extern void console_init(void);

/* Interrupts */
extern void interrupts_init(void);

/* Boot reason */
extern unsigned int hal_getBootReason(void);


void hal_init(void)
{
	hal_cpuInit();
	_imxrt_init();
	interrupts_init();

	mpu_init();
	timer_init();
	otp_init();

#if RTT_ENABLED_PLO
	rtt_init(__rttmem_rttcb);

#if ISEMPTY(UART_CONSOLE_PLO)
	/* TODO: Channel number is already hardcoded in halconsole_common.writeHook.
	Maybe let user redefine RTT channel to be used in plo? */
	lib_consoleSet(DEV_PIPE, 0);
#endif
#endif

	console_init();

	hal_common.entry = (addr_t)-1;
}


void hal_done(void)
{
	timer_done();
	_imxrt_done();

	hal_cleanDCache();
}


void hal_syspageSet(hal_syspage_t *hs)
{
	hal_common.hs = hs;
	hs->bootReason = hal_getBootReason();
}


const char *hal_cpuInfo(void)
{
	return CPU_INFO;
}


addr_t hal_kernelGetAddress(addr_t addr)
{
	return addr;
}


void hal_kernelGetEntryPointOffset(addr_t *off, int *indirect)
{
	*off = sizeof(unsigned);
	*indirect = 1;
}


void hal_kernelEntryPoint(addr_t addr)
{
	hal_common.entry = addr;
	mpu_kernelEntryPoint(addr);
}


int hal_getPartData(syspage_part_t *part, const char *imaps, size_t imapSz, const char *dmaps, size_t dmapSz)
{
	return mpu_getHalPartData(part, imaps, imapSz, dmaps, dmapSz);
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
		{ .start = (addr_t)__cmd_start, .end = (addr_t)__cmd_end, .type = hal_entryTemp },
		{ .start = (addr_t)__init_array_start, .end = (addr_t)__init_array_end, .type = hal_entryTemp },
		{ .start = (addr_t)__fini_array_start, .end = (addr_t)__fini_array_end, .type = hal_entryTemp },
		{ .start = (addr_t)__ramtext_start, .end = (addr_t)__ramtext_end, .type = hal_entryTemp },
		{ .start = (addr_t)__data_start, .end = (addr_t)__data_end, .type = hal_entryTemp },
		{ .start = (addr_t)__bss_start, .end = (addr_t)__bss_end, .type = hal_entryTemp },
		{ .start = (addr_t)__heap_base, .end = (addr_t)__heap_limit, .type = hal_entryTemp },
		{ .start = (addr_t)__stack_limit, .end = (addr_t)__stack_top, .type = hal_entryTemp },
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


int hal_cpuJump(void)
{
	if (hal_common.entry == (addr_t)-1)
		return -1;

	hal_interruptsDisableAll();

	mpu_getHalData(hal_common.hs);

	__asm__ volatile("mov r9, %1; \
		 blx %0"
		:
		: "r"(hal_common.entry), "r"((addr_t)hal_common.hs));

	return 0;
}
