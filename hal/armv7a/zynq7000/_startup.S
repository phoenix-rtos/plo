/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
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

#include "cpu.h"

.arm

.section .init, "ax"
.globl _vector_table


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

	/* Only CPU0 performs initialization, others go into wfi */
	mrc p15, 0, r1, c0, c0, 5       /* Read Multiprocessor Affinity Register */
	and r1, r1, #0xf                /* Extract CPU ID                        */
	cmp r1, #0
	beq initialize

/* TODO: make appropriate action when other core than CPU_0 is running */
wait_loop:
	wfi
	b wait_loop


initialize:
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

	/* Enable L1 Caches */
	mrc p15, 0, r1, c1, c0, 0         /* Read SCTLR (System Control Register) data  */
	orr r1, r1, #(0x1 << 2)           /* Enable data cache                          */
	orr r1, r1, #(0x1 << 12)          /* Enable instruction cache                   */
	orr r1, r1, #(0x1 << 11)          /* Enable branch prediction                   */
	bic r1, r1, #(0x1 << 28)          /* Disable TEX remap                          */
	mcr p15, 0, r1, c1, c0, 0         /* Write SCTLR (System Control Register) data */
	dsb
	isb

	/* Set Vector Table Address */
	ldr	r0, =_vector_table
	mcr	p15, 0, r0, c12, c0, 0       /* Write to VBAR (Vector Base Address Register) */


	/* Setup initial SP */
	ldr r0, =ADDR_STACK

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
	ldr r8, =plo_init
	bx r8


/* TODO */
_exception_undef:
	cpsid if
	1: b 1b


/* TODO */
_exception_prefetch:
	cpsid if
	1: b 1b


/* TODO */
_exception_abort:
	cpsid if
	1: b 1b


/* TODO */
_exceptions_dispatch:
	cpsid if
	1: b 1b


_interrupts_dispatch:
	stmfd sp, {r0-r3}            /* Store AAPCS registers and lr on to the IRQ stack   */
	mrs r2, spsr
	sub r1, lr, #4               /* Back lr to previous instruction                    */
	sub r0, sp, #0x10            /* Keep address of AAPCS registers store on IRQ stack */

	mrc p15, 0, r3, c13, c0, 4   /* Fetch SP from TPIDRPRW register                    */
	cpsie af, #SYS_MODE          /* Return to SYS mode with no interrupts              */

	/* If exception was not taken in user mode, use current stack
	 * to store context. Otherwise use preffered one from r3 */
	tst r2, #0x0f
	movne r3, sp

	stmfd r3!, {r1}              /* Save lr address */

	/* Store original r4-r14 registers (original r0-r3 are still on IRQ stack) */
	stmfd r3!, {r4-r14}
	mov sp, r3

	/* Fetch original r0-r3 from IRQ stack and store on local one
	 * including SPSR stored in current r2 */
	ldmfd r0, {r3-r6}
	push {r2-r6}

	blx interrupts_dispatch

	pop {r11}                    /* Load cpsr to r11                   */
	pop {r0-r10}
	mov r12, sp                  /* r12 points to r11, r12, sp, lr, pc */
	ldr sp, [r12, #0x8]
	ldr lr, [r12, #0xc]

	cps #IRQ_MODE                /* Change to IRQ state                */

	push {r11}                   /* Push cpsr to IRQ stack             */
	ldr r11, [r12, #0x0]
	ldr lr, [r12, #0x10]
	push {lr}
	ldr r12, [r12, #0x4]

	/* Return from exception - restores pc and cpsr for current mode */
	rfefd sp!


/* TODO */
_syscalls_dispatch:
	cpsid if
	1: b 1b
