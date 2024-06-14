/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * CPU related definitions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _SBI_HART_H_
#define _SBI_HART_H_


#include "sbi.h"


/* clang-format off */
#define RISCV_FENCE(p, s) \
	({ \
		__asm__ volatile ("fence " #p ", " #s ::: "memory"); \
	})


#define __WFI() \
	do { \
		__asm__ volatile ("wfi" ::: "memory"); \
	} while (0)
/* clang-format on */


void __attribute__((noreturn)) hart_halt(void);


void __attribute__((noreturn)) hart_changeMode(sbi_param ar0, sbi_param arg1, addr_t nextAddr, sbi_param nextMode);


void hart_init(void);


#endif
