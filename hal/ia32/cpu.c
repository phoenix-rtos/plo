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
	syspage_hal_t *syspage = (syspage_hal_t *)syspage_getAddress();
	unsigned int offs = 0;

	/* Copy HAL syspage data */
	syspage_setHalData(&_plo_syspage);
	syspage_save();

	/* Add memory map entries */
	syspage->mmsize = 0;
	do {
		if (hal_memoryGetEntry(&offs, (mem_entry_t *)(syspage->mm + syspage->mmsize)))
			break;
		syspage->mmsize++;
	} while (offs);

	/* Add program entries */
	syspage->progssz = 0;

	/* Generate fake syspage data - required for current kernel syspage compatibility */
	syspage->kernel = *(unsigned int *)((char *)syspage + sizeof(syspage_hal_t)); /* Kernel */
	syspage->kernelsz = 0xc0000;                                                  /* Kernel size */
	syspage->console = 0;                                                         /* Console */
	syspage->arg[0] = '\0';                                                       /* Args */

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
