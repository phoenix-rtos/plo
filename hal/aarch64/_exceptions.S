/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exception handler for AArch64
 *
 * Copyright 2024 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__


#include "cpu.h"


.globl _exceptions_dispatch
.type _exceptions_dispatch, %function
_exceptions_dispatch:
	stp	x18, x29, [sp, #-0x20]!
	add x18, sp, #0x20
	stp x30, x18, [sp, #0x10]
	/* x19 ~ x28 are callee-saved registers and exceptions_dispatch will save them if necessary */
	stp x16, x17, [sp, #-0x10]!
	stp x14, x15, [sp, #-0x10]!
	stp x12, x13, [sp, #-0x10]!
	stp x10, x11, [sp, #-0x10]!
	stp x8, x9, [sp, #-0x10]!
	stp x6, x7, [sp, #-0x10]!
	stp x4, x5, [sp, #-0x10]!
	stp x2, x3, [sp, #-0x10]!
	stp x0, x1, [sp, #-0x10]!
	mrs x0, spsr_el3
	mrs x1, elr_el3
	stp x0, x1, [sp, #-0x10]!

	mrs x0, esr_el3
	mrs x1, far_el3
	stp x0, x1, [sp, #-0x10]!

	/* Load arguments for exceptions_dispatch() */
	mov x0, #0
	mov x1, sp

	msr daifClr, #7

	bl exceptions_dispatch

	msr daifSet, #7

	add sp, sp, #0x10 /* Discard esr, far - not needed to restore context */
	ldp x0, x1, [sp], #0x10
	msr spsr_el3, x0
	msr elr_el3, x1
	ldp x0, x1, [sp], #0x10
	ldp x2, x3, [sp], #0x10
	ldp x4, x5, [sp], #0x10
	ldp x6, x7, [sp], #0x10
	ldp x8, x9, [sp], #0x10
	ldp x10, x11, [sp], #0x10
	ldp x12, x13, [sp], #0x10
	ldp x14, x15, [sp], #0x10
	ldp x16, x17, [sp], #0x30
	ldp x30, x29, [sp, #-0x10]
	mov x18, sp
	mov sp, x29
	ldp x18, x29, [x29, #-0x20]
	eret
.size _exceptions_dispatch, .-_exceptions_dispatch
