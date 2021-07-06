/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware abstraction layer
 *
 * Copyright 2012, 2016, 2021 Phoenix Systems
 * Copyright 2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <syspage.h>


/* Console */
extern void hal_consoleInit(void);

/* Exceptions */
extern void hal_exceptionsInit(void);

/* Interrupts */
extern void hal_interruptsInit(void);

/* Memory */
extern void hal_memoryInit(void);

/* Timer */
extern void hal_timerInit(void);


addr_t hal_kernelGetAddress(addr_t addr)
{
	return addr - VADDR_KERNEL_BASE;
}


/* Sets video mode */
static void hal_setMode(unsigned char mode)
{
	__asm__ volatile(
		"pushl $0x10; "
		"pushl $0x0; "
		"pushl $0x0; "
		"call _interrupts_bios; "
		"addl $0xc, %%esp; "
	:: "a" (mode)
	: "memory", "cc");
}


void hal_done(void)
{
	/* Restore default text mode */
	hal_setMode(0x3);
}


void hal_init(void)
{
	hal_consoleInit();
	hal_exceptionsInit();
	hal_interruptsInit();
	hal_memoryInit();
	hal_timerInit();

	/* Set graphics video mode (required for writing characters with color attribute via BIOS 0x10 interrupt) */
	hal_setMode(0x12);
}
