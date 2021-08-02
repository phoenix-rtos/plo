/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exceptions handlers for Cortex-A v. 7
 *
 * Copyright 2018, 2020, 2021 Phoenix Systems
 * Author: Pawel Pisarczyk, Aleksander Kaminski, Maciej Purski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__


#include "cpu.h"

.arm


.globl _exception_undef
.type _exception_undef, %function
_exception_undef:
	cpsid if
	stmfd sp, {r0-r4}
	mov r0, #1
	mrs r3, spsr
	tst r3, #0x20
	subeq r2, lr, #4
	subne r2, lr, #2
	b _exceptions_dispatch
.size _exception_undef, .-_exception_undef


.globl _exception_prefetch
.type _exception_prefetch, %function
_exception_prefetch:
	cpsid if
	stmfd sp, {r0-r4}
	mov r0, #3
	sub r2, lr, #4
	b _exceptions_dispatch
.size _exception_prefetch, .-_exception_prefetch


.globl _exception_abort
.type _exception_abort, %function
_exception_abort:
	cpsid if
	stmfd sp, {r0-r4}
	mov r0, #4
	sub r2, lr, #8
	b _exceptions_dispatch
.size _exception_abort, .-_exception_abort


.globl _syscalls_dispatch
.type _syscalls_dispatch, %function
_syscalls_dispatch:
	cpsid if
	stmfd sp, {r0-r4}
	mov r0, #4
	sub r2, lr, #8
	b _exceptions_dispatch
.size _syscalls_dispatch, .-_syscalls_dispatch


.globl _exceptions_dispatch
.type _exceptions_dispatch, %function
_exceptions_dispatch:
	mrs r3, spsr
	sub r1, sp, #0x14
	mrc p15, 0, r4, c13, c0, 4
	cps #SYS_MODE
	tst r3, #0x0f
	movne r4, sp
	stmfd r4!, {r2}
	stmfd r4!, {r5-r14}
	mov sp, r4
	ldmfd r1, {r4-r8}
	push {r3-r8}
	mrc p15, 0, r1, c6, c0, 2
	push {r1}
	mrc p15, 0, r1, c5, c0, 1
	push {r1}
	mrc p15, 0, r1, c6, c0, 0
	push {r1}
	mrc p15, 0, r1, c5, c0, 0
	push {r1}
	sub r1, sp, #4
	push {r1}

	ldr lr, =exceptions_dispatch
	blx lr

	ldr sp, [sp]
	add sp, sp, #20

	pop {r11}
	pop {r0-r10}
	mov r12, sp
	ldr sp, [r12, #0x8]
	ldr lr, [r12, #0xc]
	cps #IRQ_MODE
	push {r11}
	ldr r11, [r12, #0x0]
	ldr lr, [r12, #0x10]
	push {lr}
	ldr r12, [r12, #0x4]
	rfefd sp!
.size _exceptions_dispatch, .-_exceptions_dispatch
