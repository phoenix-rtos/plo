/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * NOEL-V CPU specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "csr.h"
#include "custom-csr.h"


void platform_cpuEarlyInit(void)
{
	/* Enable execution of cache management instructions */
	csr_write(CSR_MENVCFG, 0xffu);
	/* Enable I & D-cache and snooping */
	csr_write(CSR_CCTRL, (1 << 8) | 0xf);
}
