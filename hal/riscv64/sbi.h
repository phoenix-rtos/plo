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


/* Reset types */
#define SBI_RESET_TYPE_SHUTDOWN 0x0
#define SBI_RESET_TYPE_COLD     0x1
#define SBI_RESET_TYPE_WARM     0x2

/* Reset reason */
#define SBI_RESET_REASON_NONE    0x0
#define SBI_RESET_REASON_SYSFAIL 0x1


/* Legacy SBI v0.1 calls */


long sbi_putchar(int ch);


long sbi_getchar(void);


/* SBI v0.2+ calls */


sbiret_t sbi_getSpecVersion(void);


sbiret_t sbi_probeExtension(long extid);


void sbi_setTimer(u64 stime);


__attribute__((noreturn)) void sbi_reset(u32 type, u32 reason);


void sbi_init(void);


#endif
