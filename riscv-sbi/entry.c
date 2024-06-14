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

#include "sbi.h"
#include "types.h"


volatile u32 bootHartId;


void __attribute__((noreturn)) entry(u32 hartid, const void *fdt)
{
	if (hartid == bootHartId) {
		sbi_initCold(hartid, fdt);
	}
	else {
		sbi_initWarm(hartid);
	}
}
