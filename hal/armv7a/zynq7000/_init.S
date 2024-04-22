/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low-level initialization for Cortex-A9 (ARMv7) architecture
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "config.h"
#include "../cpu.h"

.arm

.section .init, "ax"
.globl _vector_table

.extern syspage_common

.global _start
.type _start, %function


/* Vector Table Definition */
.org 0
_vector_table:
	b _start
	b _exception_undef
	b _syscalls_dispatch
	b _exception_prefetch
	b _exception_abort
	.word 0
	b _interrupts_dispatch
	b _interrupts_dispatch


/* startup code */
_start:
	cpsid aif, #SYS_MODE

	/* Set Vector Table Address */
	ldr	r0, =_vector_table
	mcr	p15, 0, r0, c12, c0, 0       /* Write to VBAR (Vector Base Address Register) */

	/* Enable PMU */
	mrc p15, 0, r0, c9, c12, 0       /* Read PMCR (Performance Monitor Control Register)  */
	orr r0, #0x7                     /* Cycle counter reset - bit[2], Performance counter reset - bit[1], enable all counters - bit[0] */
	mcr p15, 0, r0, c9, c12, 0       /* Write PMCR (Performance Monitor Control Register) */
	mrc p15, 0, r0, c9, c12, 1       /* Read CESR (Count Enable Set Register)             */
	orr r0, #1 << 31                 /* Enable cycle counter                              */
	mcr p15, 0, r0, c9, c12, 1       /* Write CESR (Count Enable Set Register)            */


	/* Disable MMU */
	mrc	p15, 0, r1, c1, c0, 0        /* Read SCTLR (System Control Register) data  */
	bic r1, r1, #0x1                 /* Disable MMU                                */
	mcr	p15, 0, r1, c1, c0, 0        /* Write SCTLR (System Control Register) data */


	/* Disable L1 caches */
	mrc p15, 0, r1, c1, c0, 0        /* Read SCTLR (System Control Register) data  */
	bic r1, r1, #(0x1 << 12)         /* Disable ICache                             */
	bic r1, r1, #(0x1 << 2)          /* Disable DCache                             */
	mcr p15, 0, r1, c1, c0, 0        /* Write SCTLR (System Control Register) data */

	/* Invalidate L1 ICache */
	mov r1, #0
	mcr p15, 0, r1, c7, c5, 0        /* Clear ICIALLU */

	/* Invalidate L1 DCache. Based on ARM Cortex-A Series Programmer's Guide */
	mrc p15, 1, r0, c0, c0, 0        /* Read CCSIDR (Cache Size Identification Register) */
	mov r3, #0x1ff
	and r0, r3, r0, lsr #13          /* r0 = number of sets -                            */
	mov r1, #0                       /* r1 = way counter way_loop                        */
way_loop:
	mov r3, #0                       /* r3 = set counter set_loop                        */
set_loop:
	mov r2, r1, lsl #30
	orr r2, r3, lsl #5               /* r2 = set/way cache operation format              */
	mcr p15, 0, r2, c7, c6, 2        /* Invalidate line described by r2; write to DCISW  */
	add r3, r3, #1                   /* Increment set counter                            */
	cmp r0, r3                       /* Check whether last set was reached               */
	bgt set_loop                     /* Iterate set_loop                                 */
	add r1, r1, #1                   /* Increment way counter                            */
	cmp r1, #4                       /* Check whether last way was reached               */
	bne way_loop

	/* Invalidate TLB */
	mcr p15, 0, r1, c8, c7, 0

	/* Enable SMP */
	mrc p15, 0, r1, c1, c0, 1
	orr r1, r1, #(1 << 6)
	mcr p15, 0, r1, c1, c0, 1

	/* CPU 0 continues initialization, other cores wait for the signal to continue */
	mrc p15, 0, r1, c0, c0, 5       /* Read Multiprocessor Affinity Register */
	ands r1, r1, #0xf               /* Extract CPU ID                        */
	beq initialize

/*
	Important: the code below ONLY runs on QEMU. On real Zynq 7000 Boot ROM loads code into
	high OCRAM and then jumps CPU 1 to that code. The code below simulates the behavior of that code.
 */
	mov r0, #0xfffffff0
	ldr r1, =load_again
	str r1, [r0]
load_again:
	dsb
	wfe
	ldr lr, [r0]
	cmp lr, r1
	beq load_again
	bx	lr

initialize:
	/* Enable L1 Caches */
	mrc p15, 0, r1, c1, c0, 0         /* Read SCTLR (System Control Register) data  */
	orr r1, r1, #(0x1 << 2)           /* Enable data cache                          */
	orr r1, r1, #(0x1 << 12)          /* Enable instruction cache                   */
	orr r1, r1, #(0x1 << 11)          /* Enable branch prediction                   */
	bic r1, r1, #(0x1 << 28)          /* Disable TEX remap                          */
	mcr p15, 0, r1, c1, c0, 0         /* Write SCTLR (System Control Register) data */
	dsb
	isb

	/* Setup initial SP */
	ldr r0, =_stack
	bic r0, #7

	/* FIQ mode stack */
	msr CPSR_c, #(FIQ_MODE | NO_INT)
	mov sp, r0
	sub r0, r0, #0x20

	/* IRQ mode stack */
	msr CPSR_c, #(IRQ_MODE | NO_INT)
	mov sp, r0
	sub r0, r0, #0x100

	/* Supervisor mode stack */
	msr CPSR_c, #(SVC_MODE | NO_INT)
	mov sp, r0
	sub r0, r0, #0x40

	/* Undefined mode stack */
	msr CPSR_c, #(UND_MODE | NO_INT)
	mov sp, r0
	sub r0, r0, #0x40

	/* Abort mode stack */
	msr CPSR_c, #(ABT_MODE | NO_INT)
	mov sp, r0
	sub r0, r0, #0x40

	/* System mode stack */
	msr CPSR_c, #(SYS_MODE | NO_INT)
	mov sp, r0


	/* Jump to plo */
	ldr r8, =_startc
	bx r8
.size _start, .-_start

.globl hal_coreStart
.type hal_coreStart, %function
hal_coreStart:
	/* Enable SMP */
	mrc p15, 0, r9, c1, c0, 1
	orr r9, r9, #(1 << 6)
	mcr p15, 0, r9, c1, c0, 1

	ldr r9, =syspage_common
	ldr r9, [r9]
	ldr r8, [r9, #8] /* Jump to kernel */
	bx r8
.size hal_coreStart, .-hal_coreStart
.ltorg


#include "../_interrupts.S"
#include "../_exceptions.S"
