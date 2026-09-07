/*
 * Phoenix SBI
 *
 * Early boot routines
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "atomic.h"
#include "boot.h"
#include "config.h"


volatile u64 boot_statusArr[MAX_HART_COUNT] __attribute__((section(".bootstate")));


void boot_release(unsigned int bootHartId)
{
	size_t i;

	/* Order the image copy and the .bss clearing before the stores that
	 * release the secondary harts */
	RISCV_FENCE(rw, rw);

	for (i = 0; i < MAX_HART_COUNT; i++) {
		if ((i != bootHartId) && (ATOMIC_READ(&boot_statusArr[i]) == BOOT_WAIT_MAGIC)) {
			ATOMIC_WRITE(&boot_statusArr[i], BOOT_DONE_MAGIC);
		}
	}
	ATOMIC_WRITE(&boot_statusArr[bootHartId], BOOT_DONE_MAGIC);
}
