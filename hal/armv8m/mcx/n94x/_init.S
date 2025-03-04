/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low-level initialization for Cortex-M33 (ARMv8) architecture
 *
 * Copyright 2012, 2016-2017, 2020-2022, 2024 Phoenix Systems
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

.rept 156                   /* Max number of ext interrupts - last peripheral id + 1 */
.word _interrupts_dispatch  /* _interrupts_dispatch */
.endr
.size _init_vectors, .-_init_vectors

.org _init_vectors + 0x300

.globl _vtor_temp
.type _vtor_temp, %object
_vtor_temp:
.word _stack
.word _start_stage2
.size _vtor_temp, .-_vtor_temp


.thumb
.thumb_func

.globl _start
.type _start, %function
_start:
	cpsid if
#ifdef MCX_CPU0_RST_ADDR
	ldr     r0, =MCX_CHECK_ADDR
	mov     r1, #0
	str     r1, [r0]
	dsb
	isb
#endif
#ifdef MCX_USE_CPU1
	/* Switch to CPU1 (micro M33) */
	/* Set CPBOOT1 (CPU1 VTOR) */
	ldr r0, =_vtor_temp
	ldr r1, =0x40000800
	str r0, [r1, #4]

	/* Start CPU1 */
	ldr r0, =0xc0c40008
	str r0, [r1]

#ifdef MCX_CPU0_RST_ADDR
	ldr     r0, =MCX_CHECK_ADDR
wait_loop:
	wfe
	dsb
	ldr     r1, [r0]
	cmp     r1, #1
	bne     wait_loop

/* Jump to core0 prog */
	ldr     r0, =MCX_CPU0_RST_ADDR
	ldr     r0, [r0]
	bx      r0
#else
	/* CPU0: Wait for something to do */
	/* FIXME: might be not possible to
	 * exit this situation, CPU0 reset might
	 * cause global reset. How to park it
	 * in a way, that allows to reuse the CPU? */
1:	wfi
	b 1b
#endif

#else
	b _start_stage2
#endif

.size _start, .-_start
.ltorg


.globl _start_stage2
.type _start_stage2, %function
_start_stage2:
	cpsid if

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
.size _start_stage2, .-_start_stage2
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
	/* TODO: implement exception dispatcher */
	1: b 1b
.size _exceptions_dispatch, .-_exceptions_dispatch
.ltorg
