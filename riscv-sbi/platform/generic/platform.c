/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * RISC-V Generic CPU specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "csr.h"


void platform_cpuEarlyInit(void)
{
	/* Quick and dirty PMP configuration: full memory range RWX for S/U */
	csr_write(CSR_PMPADDR0, -1);
	csr_write(CSR_PMPCFG0, 0x0f);
}
