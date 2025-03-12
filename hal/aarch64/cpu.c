/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * AArch64 CPU and cache management
 *
 * Copyright 2024 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#include "cache.h"


void hal_interruptsDisableAll(void)
{
	asm volatile("msr daifSet, #7");
}


void hal_interruptsEnableAll(void)
{
	asm volatile("msr daifClr, #7");
}


void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz)
{
	switch (type) {
		case hal_cpuDCache:
			hal_dcacheInval(addr, addr + sz);
			break;

		case hal_cpuICache:
			/* TODO */
			break;

		default:
			break;
	}
}
