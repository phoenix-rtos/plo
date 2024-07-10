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


/* Load value using mstatus privilege modification
 * This macro allows loads from virtual address in M-Mode.
 */
#define MPRV_LOAD(load_inst, reg, addr) \
	__asm__ volatile ( \
		/* Set MPRV bit */ \
		"li a6, (1 << 17)\n\t" \
		"csrs mstatus, a6\n\t" \
		".option push\n\t" \
		".option norvc\n\t" \
		#load_inst " %0, (%1)\n\t" \
		".option pop\n\t" /* Clear MPRV bit */ \
		"csrc mstatus, a6" \
		: "=r"(reg) \
		: "r"(addr) \
		: "a6", "memory" \
	);

/* clang-format on */


void __attribute__((noreturn)) hart_halt(void);


void __attribute__((noreturn)) hart_changeMode(sbi_param ar0, sbi_param arg1, addr_t nextAddr, sbi_param nextMode);


void hart_init(void);


#endif
