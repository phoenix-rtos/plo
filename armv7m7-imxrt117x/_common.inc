/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low-level initialization for Cortex-M7 (ARMv7) architecture - common code
 *
 * Copyright 2012, 2016-2017, 2020-2021 Phoenix Systems
 * Author: Jacek Popko, Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#define IOMUXC_GPR 0x400e4000

#define ADDR_DTCM 0x20000000
#define ADDR_SCB  0xe000ed00

.globl _init_vectors
.type _init_vectors, %object
_init_vectors:
.word _end + 5 * 1024
.word _start

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

	/* Configure FLEX RAM */
	ldr r0, =IOMUXC_GPR

	/* Low */
	ldr r1, = 0xaaaa
	str r1, [r0, #0x44]

	/* HIGH */
	ldr r1, =0xffff
	str r1, [r0, #0x48]

	/* Use provided FLEXRAM config */
	ldr r1, [r0, #0x40]
	orr r1, #(1 << 2)
	str r1, [r0, #0x40]

	/* Init vector table and stack pointer */
	ldr r0, =0xe000ed08
	ldr r1, =_init_vectors

	str r1, [r0]
	ldr r0, [r1]
	msr msp, r0
	ldr r8, =plo_init
	bx r8

.size _start, .-_start
.ltorg


.globl _interrupts_dispatch
.type _interrupts_dispatch, %function
_interrupts_dispatch:
	cpsid if

	mrs r0, ipsr
	sub r1, sp, #44
	stmdb sp!, {r1-r2, r4-r11, lr}

_intd0:
	bl low_irqdispatch
	ldr sp, [sp]
_intd1:
	ldmia sp!, {r0-r1, r4-r11, lr}

	cpsie if
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
