/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * GRFPGA specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "csr.h"
#include "platform/gaisler/custom-csr.h"


void platform_cpuEarlyInit(void)
{
	/* Enable execution of cache management instructions */
	csr_write(CSR_MENVCFG, 0x70);
	/* Enable I & D-cache and snooping */
	csr_write(CSR_CCTRL, (1 << 8) | 0xf);
	/* Enable performance features (branch prediction, etc.) */
	csr_write(CSR_FEATURES, 0);
	/* Quick PMP configuration: covers full 56-bit space */
	csr_write(CSR_PMPADDR0, 0x3FFFFFFFFFFFFFFFULL);
	/* NAPOT mode (A=0b11), RWX for S/U */
	csr_write(CSR_PMPCFG0, 0x1f);
}
