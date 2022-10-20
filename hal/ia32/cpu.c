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


struct {
	addr_t entry;
} cpu_common;


extern addr_t hal_memoryGetSyspageAddr(void);


void hal_kernelEntryPoint(addr_t addr)
{
	cpu_common.entry = addr;
}


int hal_cpuJump(void)
{
	addr_t syspage = hal_memoryGetSyspageAddr();

	if (cpu_common.entry == (addr_t)-1)
		return -1;

	__asm__ volatile(
		"cli; "
		"movl 24(%0), %%esp; "
		"pushl %0; "
		"jmpl *%1; "
	:: "g" (syspage), "r" (cpu_common.entry));

	return 0;
}


void hal_cpuReboot(void)
{
	u8 status;

	hal_interruptsDisableAll();

	/* 1. Try to reboot using keyboard controller (8042) */
	do {
		status = hal_inb((void *)0x64);
		if (status & 1)
			(void)hal_inb((void *)0x60);
	} while (status & 2);
	hal_outb((void *)0x64, 0xfe);

	/* 2. Try to reboot by PCI reset */
	hal_outb((void *)0xcf9, 0xe);

	/* 3. Triple fault (interrupt with null idt) */
	__asm__ volatile(
		"lidt _plo_idtr_empty; "
		"int3; ");

	/* 4. Nothing worked, halt */
	for (;;)
		hal_cpuHalt();

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
