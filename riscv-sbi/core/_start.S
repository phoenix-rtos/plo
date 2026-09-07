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


#include "boot.h"
#include "config.h"
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

#ifndef FDT_PATH
	/* Means a1 holds FDT address, save it */
	mv s1, a1
#endif

	mv a1, zero
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

/* Don't use gp relaxations, we might be running from flash while linked to ram */
.option push
.option norelax
	csrr s0, CSR_MHARTID

	/* Invalid HART ID */
	li t1, MAX_HART_COUNT
	bgeu s0, t1, _hart_park

	li t0, BOOT_HART_ID

	/* only boot HART continues */
	bne s0, t0, _wait_boot_hart

	/* t2 = current address - link address */
	lla t1, _start
	lla t2, _link_start
	ld t2, (t2)
	sub t2, t1, t2

	/* Already running at the link address, nothing to copy */
	beqz t2, _in_ram

	/* Copy the SBI + payload. Both regions are 8 byte aligned through linkerscript. */
	lla a1, _copy_table
	lla a5, _copy_table_end

_copy_region:
	ld a2, 0(a1)   /* destination start */
	ld a3, 8(a1)   /* destination end */
	add a4, a2, t2 /* source */
	beq a2, a3, _copy_next

_copy_word:
	ld a6, (a4)
	sd a6, (a2)
	addi a4, a4, 8
	addi a2, a2, 8
	bltu a2, a3, _copy_word

_copy_next:
	addi a1, a1, 16
	bltu a1, a5, _copy_region

	/* Make the copied instructions visible to our own instruction fetch */
	fence rw, rw
	fence.i

_jump_to_ram:
	/* Absolute jump */
	lla a1, _in_ram_addr
	ld a1, (a1)
	jr a1

_in_ram:
	la gp, __global_pointer$
.option pop

	/* Set the stack pointer */
	la sp, _stack
	slli t3, s0, 12 /* 4KB stack */
	add sp, sp, t3

	li t0, BOOT_HART_ID
	bne s0, t0, _boot_continue

	/* Load FDT addr */
#ifdef FDT_PATH
	la s1, __fdt_start
#endif /* else s1 already has the fdt address */

	/* Clear the .bss section */
	la a0, __bss_start
	li a1, 0
	la a2, __bss_end
	sub a2, a2, a0

	call sbi_memset

	/* Release the secondary harts */
	mv a0, s0
	call boot_release

_boot_continue:
	/* Mark ourselves in the hart mask */
	la a1, sbi_hartMask
	li a2, 1
	sll a3, a2, s0
	amoor.d.aq zero, a3, (a1)

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

.option push
.option norelax
_wait_boot_hart:
	lla a1, _boot_status_arr
	ld a1, (a1)

	/* a1 = &boot_statusArr[hartid] */
	slli a2, s0, 3
	add a1, a1, a2

	lla a3, _boot_wait_magic
	ld a3, (a3)

	lla a4, _boot_done_magic
	ld a4, (a4)

	/* This one's ready - write WAIT_MAGIC */
	sd a3, (a1)
	fence rw, rw

1:
	ld a3, (a1)
	fence r, r
	bne a3, a4, 1b

	/* The image and .bss are ready */
	fence r, r
	fence.i

	j _jump_to_ram

_hart_park:
	wfi
	j _hart_park
.option pop
.size _start, . - _start


.align 3
_link_start:
	.dword __init_start

_in_ram_addr:
	.dword _in_ram

_boot_wait_magic:
	.dword BOOT_WAIT_MAGIC

_boot_done_magic:
	.dword BOOT_DONE_MAGIC

_boot_status_arr:
	.dword boot_statusArr

/* Regions to copy when running from flash, as (start, end) link addresses */
.align 3
_copy_table:
	.dword _sbi_start
	.dword _sbi_end
	.dword _payload_start
	.dword _payload_end
_copy_table_end:

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
