/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv8 Cortex-R
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "hal/armv8r/cache.h"


void hal_interruptsDisableAll(void)
{
	__asm__ volatile("cpsid if");
}


void hal_interruptsEnableAll(void)
{
	__asm__ volatile("cpsie if");
}


void hal_cpuReboot(void)
{
	/* TODO */
	for (;;) {
	}
}


void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz)
{
	switch (type) {
		case hal_cpuDCache:
			hal_dcacheInval(addr, addr + sz);
			break;

		case hal_cpuICache:
			hal_icacheInval();
			break;

		default:
			break;
	}
}
