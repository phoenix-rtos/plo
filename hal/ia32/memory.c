/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Memory map
 *
 * Copyright 2012, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include <syspage.h>


/* Flushes 8042 keyboard controller buffers */
static inline void memory_empty8042(void)
{
	unsigned char status;

	do {
		/* Discard input data */
		if ((status = hal_inb((void *)0x64)) & 0x01) {
			hal_inb((void *)0x60);
			continue;
		}
	} while (status & 0x02);
}


/* Enables A20 line mask */
static void memory_enableA20(void)
{
	memory_empty8042();
	hal_outb((void *)0x64, 0xd1);
	memory_empty8042();
	hal_outb((void *)0x60, 0xdf);
	memory_empty8042();
}


int hal_memoryGetEntry(unsigned int *offs, mem_entry_t *entry)
{
	int ret = ((unsigned int)entry & 0xffff0000) >> 4;
	unsigned int next = *offs;

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
	: "D" (entry)
	: "ecx", "edx", "memory", "cc");
	*offs = next;

	return ret;
}


int hal_memoryInit(void)
{
	memory_enableA20();

	/* Initialize syspage */
	syspage_init();
	syspage_setAddress(ADDR_SYSPAGE);

	/* FIXME: add correct memory entries after new syspage implementation is added */
	/* Add loader entries */
	// syspage_addEntries(0x0, 0x1000);
	// syspage_addEntries(ADDR_GDT, ADDR_SYSPAGE - ADDR_GDT);
	// syspage_addEntries(ADDR_PDIR, ADDR_STACK - ADDR_PDIR);
	// syspage_addEntries(ADDR_PLO, (addr_t)_end - ADDR_PLO);
	// syspage_addEntries(ADDR_RCACHE, SIZE_RCACHE + SIZE_WCACHE);

	/* Create system memory map */
	// for (;;) {
	// 	if (memory_getEntry(&offs, &entry))
	// 		return -EFAULT;

	// 	/* Add reserved entries */
	// 	/* TODO: fix max addr overflow (addr + len) */
	// 	if (entry.type != 0x1)
	// 		syspage_addEntries(entry.addr, (entry.addr + entry.len <= 0xffffffff) ? entry.len : entry.len - 1);

	// 	if (!offs)
	// 		break;
	// }

	return EOK;
}
