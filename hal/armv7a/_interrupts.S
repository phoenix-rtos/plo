/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupts handlers for Cortex-A v. 7
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

.globl _interrupts_dispatch
.type _interrupts_dispatch, %function
_interrupts_dispatch:
	stmfd sp, {r0-r3}            /* Store AAPCS registers and lr on to the IRQ stack   */
	mrs r2, spsr
	sub r1, lr, #4               /* Back lr to previous instruction                    */
	sub r0, sp, #0x10            /* Keep address of AAPCS registers store on IRQ stack */

	cpsie af, #SYS_MODE          /* Return to SYS mode with no interrupts              */

	mov r3, sp
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

.size _interrupts_dispatch, .-_interrupts_dispatch
