/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low-level initialization for Cortex-M55 (ARMv8) architecture
 *
 * Copyright 2012, 2016-2017, 2020-2025, 2026 Phoenix Systems
 * Author: Jacek Popko, Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "config.h"

.syntax unified

.section .init, "x"

.globl _init_vectors
.type _init_vectors, %object
_init_vectors:
.word _stack
.word _start

.word _exceptions_dispatch  /* NMI */
.word _exceptions_dispatch  /* HardFault */
.word _exceptions_dispatch  /* MemMgtFault */
.word _exceptions_dispatch  /* BusFault */
.word _exceptions_dispatch  /* UsageFault */
.word _exceptions_dispatch  /* SecureFault */
.word 0
.word 0
.word 0
.word 0                     /* SVC */
.word 0                     /* Debug */
.word 0
.word _interrupts_dispatch  /* PendSV */
.word _interrupts_dispatch  /* Systick */

.rept 135                   /* Max number of ext interrupts - last peripheral id + 1 */
.word _interrupts_dispatch  /* _interrupts_dispatch */
.endr
.size _init_vectors, .-_init_vectors

.thumb
.thumb_func

.globl _start
.type _start, %function
_start:
	cpsid if

	/* Set vector table pointer */
	ldr r0, =0xe000ed08 /* SCB_VTOR */
	ldr r3, =_init_vectors
	str r3, [r0]
	dsb

	/* Set stack pointer and stack limit register */
	ldr r0, [r3]
	bic r0, #0x7
	msr msp, r0
	ldr r1, =_stacksz
	sub r0, r1
	msr msplim, r0

	blx _startc
.size _start, .-_start


.globl _interrupts_dispatch
.type _interrupts_dispatch, %function
_interrupts_dispatch:
	mrs r0, ipsr
	stmdb sp!, {r0, r4-r11, lr}
	bl hal_interruptDispatch
	ldmia sp!, {r0, r4-r11, lr}
	dmb

	bx lr
.size _interrupts_dispatch, .-_interrupts_dispatch
.ltorg


.globl _exceptions_dispatch
.type _exceptions_dispatch, %function

_exceptions_dispatch:
	cpsid if
	mrs r0, ipsr
	/* TODO: implement exception dispatcher */
	1: b 1b
.size _exceptions_dispatch, .-_exceptions_dispatch
.ltorg
