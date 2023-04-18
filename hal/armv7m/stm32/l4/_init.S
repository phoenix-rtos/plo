/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low-level initialization for Cortex-M7 (ARMv7) architecture
 *
 * Copyright 2012, 2016-2017, 2020, 2021 Phoenix Systems
 * Author: Jacek Popko, Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski
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
.word _start - _init_vectors

.word _exceptions_dispatch  /* NMI */
.word _exceptions_dispatch  /* HardFault */
.word _exceptions_dispatch  /* MemMgtFault */
.word _exceptions_dispatch  /* BusFault */
.word _exceptions_dispatch  /* UsageFault */
.word 0
.word 0
.word 0
.word 0
.word 0                     /* SVC */
.word 0                     /* Debug */
.word 0
.word _interrupts_dispatch  /* PendSV */
.word _interrupts_dispatch  /* Systick */

.rept 217
.word _interrupts_dispatch  /* _interrupts_dispatch */
.endr
.size _init_vectors, .-_init_vectors

.thumb
.thumb_func

.globl _start
.type _start, %function
_start:
	cpsid if

	/* Skip plo copy if we're already in RAM (i.e. loaded via JTAG) */
	mov r0, pc
	ldr r1, =_init_vectors
	cmp r0, r1
	bgt stage1

	/* Copy plo to RAM */
	ldr r0, =_init_vectors /* dest */
	ldr r1, =_plo_size
	add r1, r1, r0         /* end */
	mov r2, #0             /* src */

stage0loop:
	ldr r3, [r2], #4
	str r3, [r0], #4
	cmp r0, r1
	blt stage0loop

stage1:
	/* Init vector table and stack pointer */
	ldr r0, =0xe000ed08
	ldr r1, =_init_vectors
	str r1, [r0]
	isb
	dmb

	ldr r0, [r1]
	bic r0, 7
	msr msp, r0
	ldr r8, =_startc
	bx r8
.size _start, .-_start
.ltorg


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

	mrs r0, psp
	stmdb sp!, {r0, r4-r11, lr}

	mrs r0, ipsr
	mov r1, sp

	b hal_exceptionsDispatch
.size _exceptions_dispatch, .-_exceptions_dispatch
.ltorg
