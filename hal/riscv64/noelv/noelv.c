/*
 * Phoenix-RTOS
 *
 * Operating system loader
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

#include "noelv.h"
#include "hal/riscv64/cpu.h"
#include "hal/riscv64/csr.h"


extern void console_init(void);


void noelv_init(void)
{
	console_init();
	/* Enable execution of cache management instructions */
	csr_write(CSR_SENVCFG, 0xffu);
}
