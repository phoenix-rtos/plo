/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * SBI routines (RISCV64)
 *
 * Copyright 2018, 2020, 2024 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_SBI_H_
#define _HAL_SBI_H_

#include <types.h>


typedef struct {
	long error;
	long value;
} sbiret_t;


/* Legacy SBI v0.1 calls */


long sbi_putchar(int ch);


long sbi_getchar(void);


/* SBI v0.2+ calls */


sbiret_t sbi_getSpecVersion(void);


sbiret_t sbi_probeExtension(long extid);


void sbi_setTimer(u64 stime);


void sbi_init(void);


#endif
