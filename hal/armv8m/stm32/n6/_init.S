/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low-level initialization for Cortex-M55 (ARMv8) architecture
 *
 * Copyright 2012, 2016-2017, 2020-2025 Phoenix Systems
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

.rept 195                   /* Max number of ext interrupts - last peripheral id + 1 */
.word _interrupts_dispatch  /* _interrupts_dispatch */
.endr
.size _init_vectors, .-_init_vectors

.thumb
.thumb_func

#define SCB_VTOR           0xe000ed08
#define SCB_AIRCR          0xe000ed0c
#define RCC_APB4HENSR      (0x56028000 + 0xa78)
#define SYSCFG             0x56008000
#define SYSCFG_CM55TCMCR   0x008
#define SYSCFG_INITSVTORCR 0x010
#define SYSCFG_CM55RSTCR   0x018

.globl _start
.type _start, %function
_start:
	cpsid if
	/* Set vector table pointer */
	ldr r0, =SCB_VTOR
	ldr r3, =_init_vectors
	str r3, [r0]
	dsb

	/* Enable SYSCFG module */
	ldr r0, =RCC_APB4HENSR
	mov r1, #1
	str r1, [r0]
	dsb

	ldr r8, =SYSCFG
	/* Set vector table after reset */
	str r3, [r8, SYSCFG_INITSVTORCR]

	/* Check FLEXMEM configuration and set it up if necessary */
	ldr r0, =FLEXRAM_ITCM_BANKS
	cmp r0, #1
	bls .Lflex_itcm_01
	cmp r0, #3
	beq .Lflex_itcm_3
	b . /* Unknown configuration, hang system */

.Lflex_itcm_3:
	sub r0, #1
.Lflex_itcm_01:
	add r1, r0, #7

	ldr r0, =FLEXRAM_DTCM_BANKS
	cmp r0, #1
	bls .Lflex_dtcm_01
	b . /* Unknown configuration, hang system */
.Lflex_dtcm_01:
	add r0, #8
	orr r0, r1, r0, lsl #4

	ldr r2, [r8, SYSCFG_CM55TCMCR]
	and r2, #0xff
	cmp r2, r0
	bne _tcm_setup

	/* Set stack pointer and stack limit register */
	ldr r0, [r3]
	bic r0, #0x7
	msr msp, r0
	ldr r1, =_stacksz
	sub r0, r1
	msr msplim, r0
	ldr r8, =_startc
	bx r8
.size _start, .-_start


/* r8 - SYSCFG base pointer
 * r0 - target FLEXRAM config */
.type _tcm_setup, %function
_tcm_setup:
	/* Store target TCM configuration */
	ldr r2, [r8, SYSCFG_CM55TCMCR]
	bic r2, #0xff
	orr r2, r0
	str r2, [r8, SYSCFG_CM55TCMCR]

	/* Set reset type to power-on reset */
	ldr r0, [r8, SYSCFG_CM55RSTCR]
	orr r0, #1
	str r0, [r8, SYSCFG_CM55RSTCR]
	dsb

	/* Perform reset */
	ldr r0, =SCB_AIRCR
	ldr r1, [r0]
	movt r1, #0x05fa
	orr r1, #(1 << 2)
	str r1, [r0]
	dsb

	b . /* Hang and wait for reset */
.size _tcm_setup, .-_tcm_setup
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
	mrs r0, ipsr
	/* TODO: implement exception dispatcher */
	1: b 1b
.size _exceptions_dispatch, .-_exceptions_dispatch
.ltorg
