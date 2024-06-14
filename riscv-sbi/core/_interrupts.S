/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * Interrupt stubs
 *
 * Copyright 2023, 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__


#include "csr.h"

.text

.align 2

.global _interrupts_dispatch
.type _interrupts_dispatch, @function
_interrupts_dispatch:
	csrrw a1, CSR_MSCRATCH, a1  /* a1 = &perHartData[hartid] */
	sd a0, 8(a1)                /* Save a0 */

	csrr a0, CSR_MSTATUS

	/* Determine in which mode we were executing before interrupt
	 * MPP = 3 -> machine mode
	 */
	srli a0, a0, 11
	xori a0, a0, 3
	bnez a0, 1f

	/* Machine mode */
	sd sp, -272(sp)

	addi sp, sp, -280
	j 2f

1:
	/* U/S mode
	 * Load machine stack pointer from hart data
	 */
	ld a0, (a1)

	/* Save task's stack pointer */
	sd sp, -272(a0)

	/* Swap to machine stack */
	addi sp, a0, -280

2:
	/* restore a0, a1 */
	ld a0, 8(a1)
	csrrw a1, CSR_MSCRATCH, a1

	/* Save context */
	sd ra, (sp)
	sd gp, 16(sp)
	sd tp, 24(sp)
	sd t0, 32(sp)
	sd t1, 40(sp)
	sd t2, 48(sp)
	sd s0, 56(sp)
	sd s1, 64(sp)
	sd a0, 72(sp)
	sd a1, 80(sp)
	sd a2, 88(sp)
	sd a3, 96(sp)
	sd a4, 104(sp)
	sd a5, 112(sp)
	sd a6, 120(sp)
	sd a7, 128(sp)
	sd s2, 136(sp)
	sd s3, 144(sp)
	sd s4, 152(sp)
	sd s5, 160(sp)
	sd s6, 168(sp)
	sd s7, 176(sp)
	sd s8, 184(sp)
	sd s9, 192(sp)
	sd s10, 200(sp)
	sd s11, 208(sp)
	sd t3, 216(sp)
	sd t4, 224(sp)
	sd t5, 232(sp)
	sd t6, 240(sp)

	csrr s0, CSR_MSTATUS
	csrr s1, CSR_MEPC
	csrr s2, CSR_MCAUSE

	sd s0, 248(sp)   /* mstatus */
	sd s1, 256(sp)   /* mepc */
	sd s2, 264(sp)   /* mcause */

	/* Check interrupt source */
	li t0, MCAUSE_INTR
	and t1, s2, t0
	andi s0, s2, MCAUSE_IRQ_MSK /* Exception code */
	beqz t1, _interrupts_notIrq

_interrupts_dispatchIntr:
	mv a0, s0
	call interrupts_dispatch
	tail _interrupts_restoreAll

_interrupts_notIrq:
	li s2, MCAUSE_S_ECALL
	bne s0, s2, _interrupts_dispatchExc

	/* s1 = mepc, move past ecall instruction */
	addi s1, s1, 4
	sd s1, 256(sp)

	/* a0-a7 are preserved */
	call sbi_dispatchEcall
	tail _interrupts_restoreAfterEcall

_interrupts_dispatchExc:
	csrr s1, CSR_MTVAL
	sd s1, 272(sp)   /* mtval */

	mv a0, s0
	mv a1, sp
	call exceptions_dispatch

_interrupts_restoreAll:
	ld a0, 72(sp)
	ld a1, 80(sp)

_interrupts_restoreAfterEcall:
	ld s0, 248(sp)
	csrw CSR_MSTATUS, s0

	ld s0, 256(sp)
	csrw CSR_MEPC, s0

	ld ra, (sp)
	ld gp, 16(sp)
	ld tp, 24(sp)
	ld t0, 32(sp)
	ld t1, 40(sp)
	ld t2, 48(sp)
	ld s0, 56(sp)
	ld s1, 64(sp)
	ld a2, 88(sp)
	ld a3, 96(sp)
	ld a4, 104(sp)
	ld a5, 112(sp)
	ld a6, 120(sp)
	ld a7, 128(sp)
	ld s2, 136(sp)
	ld s3, 144(sp)
	ld s4, 152(sp)
	ld s5, 160(sp)
	ld s6, 168(sp)
	ld s7, 176(sp)
	ld s8, 184(sp)
	ld s9, 192(sp)
	ld s10, 200(sp)
	ld s11, 208(sp)
	ld t3, 216(sp)
	ld t4, 224(sp)
	ld t5, 232(sp)
	ld t6, 240(sp)

	/* Restore task's stack pointer */
	ld sp, 8(sp)

	mret
.size _interrupts_dispatch, .-_interrupts_dispatch
