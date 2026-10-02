/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * Main
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "config.h"
#include "sbi.h"
#include "types.h"


void __attribute__((noreturn)) entry(u32 hartid, const void *fdt)
{
	if (hartid == BOOT_HART_ID) {
		sbi_initCold(hartid, fdt);
	}
	else {
		sbi_initWarm(hartid);
	}
}
