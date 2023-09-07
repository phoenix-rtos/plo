/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low level initialization
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "cpu.h"

.align 8

.section .init, "ax"

.globl dtbAddr
dtbAddr:
	.dword

.globl _start
.type _start, @function
_start:
	la t0, dtbAddr
	sd a1, (t0)
	/* Mask all interrupts */
	csrw sie, zero

.option push
.option norelax
	la gp, __global_pointer$
.option pop

	/* Disable FPU */
	li t0, SSTATUS_FS
	csrc sstatus, t0

	la sp, _stack
	j _startc
.size _start, . - _start
