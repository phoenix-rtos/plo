/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * Low level initialization
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__


#include "csr.h"
#include "sbi.h"

.extern __bss_start
.extern __bss_end

.section ".init", "ax"

.align 2

.globl _start
.type _start, @function
_start:
	/* Disable interrupts */
	csrw CSR_MIE, zero
	csrw CSR_MIP, zero

	/* Clear/set registers */
	mv ra, zero
	mv sp, zero
	mv gp, zero
	mv tp, zero
	mv t0, zero
	mv t1, zero
	mv t2, zero
	mv s0, zero
	mv s1, zero
	mv a0, zero
#ifdef FDT_PATH
	la a1, __fdt_start
#endif /* else assume a1 contains the fdt address */

	mv a2, zero
	mv a3, zero
	mv a4, zero
	mv a5, zero
	mv a6, zero
	mv a7, zero
	mv s2, zero
	mv s3, zero
	mv s4, zero
	mv s5, zero
	mv s6, zero
	mv s7, zero
	mv s8, zero
	mv s9, zero
	mv s10, zero
	mv s11, zero
	mv t3, zero
	mv t4, zero
	mv t5, zero
	mv t6, zero
	csrw CSR_MSCRATCH, zero

.option push
.option norelax
	la gp, __global_pointer$
.option pop

	csrr a0, CSR_MHARTID

	mv s1, a1 /* save fdt address */

	/* Set the stack pointer */
	la sp, _stack
	slli a1, a0, 12 /* 4KB stack */
	add sp, sp, a1

	la a1, hartLock
	li a2, 1
	amoadd.w.aq a3, a2, (a1)
	bnez a3, _wait_boot_hart

	/* Clear the .bss section */
	la a0, __bss_start
	li a1, 0
	la a2, __bss_end
	sub a2, a2, a0

	call sbi_memset

	csrr s0, CSR_MHARTID
	la a1, bootHartId
	sw s0, (a1)

	fence w, rw

	la a1, bootReady
	li a2, 1
	amoadd.w.aqrl zero, a2, (a1)

_boot_continue:
	/* Mark ourselves in the hart mask */
	la a1, sbi_hartMask
	sll a3, a2, s0
	amoor.w.aq zero, a3, (a1)

	mv a0, s0
	mv a1, s1
	mv a2, sp
	call sbi_scratchInit

	/* Set interrupt vector */
	la a0, _interrupts_dispatch
	csrw CSR_MTVEC, a0

	/* Quickly initialize some basic cpu features (e.g. cache) */
	call platform_cpuEarlyInit

	mv a0, s0
	mv a1, s1
	j entry

_wait_boot_hart:
	/* Wait for the boot hart to finish clearing the .bss section */
	la a1, bootReady
	fence w, r
	lw a2, (a1)
	beqz a2, _wait_boot_hart

	csrr s0, CSR_MHARTID
	li a2, 1
	j _boot_continue
.size _start, . - _start


.align 2
hartLock:
	.word 0

.align 2
bootReady:
	.word 0


#ifdef FDT_PATH

.section .rodata
.align 4
.globl __fdt_start
__fdt_start:
	.incbin FDT_PATH

#endif


.section ".payload", "ax", %progbits
.align 4
.globl __payload_start
__payload_start:
	.incbin PAYLOAD_PATH
