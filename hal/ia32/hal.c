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


void hal_done(void)
{
	return;
}


void hal_init(void)
{
	hal_consoleInit();
	hal_exceptionsInit();
	hal_interruptsInit();
	hal_memoryInit();
	hal_timerInit();
}
