/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * CPU related routines
 *
 * Copyright 2021 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <syspage.h>


/* FIXME: temporary usage until new syspage implementation */
extern int hal_memoryGetEntry(unsigned int *offs, mem_entry_t *entry);


__attribute__((noreturn)) void hal_cpuJump(addr_t addr)
{
	/* FIXME: temporary HAL syspage usage, should be removed after new syspage implementation is added */
	void *syspage = (void *)syspage_getAddress();
	unsigned short *mmsz = syspage + 300;
	mem_entry_t *mm = (mem_entry_t *)(mmsz + 1);
	unsigned short *progssz = (unsigned short *)(mm + 64);
	unsigned int offs = 0;

	/* Copy HAL syspage data */
	syspage_setHalData(&_plo_syspage);
	syspage_save();

	/* Add memory map entries */
	*mmsz = 0;
	do {
		if (hal_memoryGetEntry(&offs, mm))
			break;
		(*mmsz)++;
		mm++;
	} while (offs);
	hal_memset(mm, 0, (64 - *mmsz) * sizeof(mem_entry_t));

	/* Add program entries */
	*progssz = 0;

	/* Generate fake syspage data - required for current kernel syspage compatibility */
	*(unsigned int *)(syspage + 0x20) = *(unsigned int *)(syspage + sizeof(syspage_hal_t)); /* Kernel */
	*(unsigned int *)(syspage + 0x24) = 0xc0000;                                            /* Kernel size */
	*(unsigned int *)(syspage + 0x28) = 0x0;                                                /* Console */
	*(char *)(syspage + 0x2c) = '\0';                                                       /* Args */

	hal_done();

	__asm__ volatile(
		"cli; "
		"movl 24(%0), %%esp; "
		"pushl %0; "
		"jmpl *%1; "
	:: "g" (syspage), "r" (addr));

	__builtin_unreachable();
}


void hal_cpuHalt(void)
{
	__asm__ volatile("hlt":);
}


void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz)
{
	return;
}


const char *hal_cpuInfo(void)
{
	return "IA-32 Generic";
}
