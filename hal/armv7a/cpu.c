/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv7 Cortex - A
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


void hal_interruptsDisableAll(void)
{
	__asm__ volatile("cpsid if");
}


void hal_interruptsEnableAll(void)
{
	__asm__ volatile("cpsie if");
}


void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz)
{
	switch (type) {
		case hal_cpuDCache:
			/* TODO */
		case hal_cpuICache:
			/* TODO */
		default:
			break;
	}
}
