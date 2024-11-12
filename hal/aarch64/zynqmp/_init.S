/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low-level initialization for ZynqMP (Cortex-A53 AArch64) architecture
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "config.h"
#include "../cpu.h"

/* If set to 1, PLO will run in EL1 secure mode, otherwise in non-secure mode */
#define EL1_SECURE 0

.section .init, "ax"

.extern hal_common
.extern hal_coreJumpFlag

/* startup code */
.globl _start
.type _start, %function
_start:
	/* Initialize all registers  */
	mov x0, #0
	mov x1, #0
	mov x2, #0
	mov x3, #0
	mov x4, #0
	mov x5, #0
	mov x6, #0
	mov x7, #0
	mov x8, #0
	mov x9, #0
	mov x10, #0
	mov x11, #0
	mov x12, #0
	mov x13, #0
	mov x14, #0
	mov x15, #0
	mov x16, #0
	mov x17, #0
	mov x18, #0
	mov x19, #0
	mov x20, #0
	mov x21, #0
	mov x22, #0
	mov x23, #0
	mov x24, #0
	mov x25, #0
	mov x26, #0
	mov x27, #0
	mov x28, #0
	mov x29, #0
	mov x30, #0

	/* check current exception level and drop down to EL1 if necessary */
	mrs	x0, currentEL
	cbz x0, error_loop
	cmp	x0, #0x4
	beq	continue_el1
	cmp x0, #0x8
	beq continue_el2

	/* Interesting bits:
		EE == 0 => little-endian at EL3
		WXN == 0 => write permission doesn't imply execute never
		SA == 1 => alignment checks for SP accesses
		A == 0 => disable data alignment checks
		C, I, M == 0 => disable D-cache, I-cache, MMU (for now)
	 */
	ldr x0, =0x30c50838
	msr sctlr_el3, x0

	/* Note: setting to a value of 0 will trap execution of SME instructions */
	msr cptr_el3, xzr

	/* Allow write access to various registers from EL2 */
	mov x0, #0b1110011
	msr actlr_el3, x0

	/* Invalidate TLBs at EL3 */
	tlbi alle3

	/* Initialize EL2 registers to safe values */
	msr cptr_el2, xzr
	ldr x0, =0x30c00838
	msr sctlr_el2, x0
	msr sctlr_el1, xzr
	msr vttbr_el2, xzr

	/* Allow write access to various registers from EL1 */
	mov x0, #0b1110011
	msr actlr_el2, x0
	mov x0, #((1 << 31) | (1 << 29)) /* EL1 Execution state is AArch64, disable HVC instruction */
	msr hcr_el2, x0

	/* Secure GIC initialization */
	ldr x0, =(GIC_BASE_ADDRESS)
	ldr w1, [x0, #4]    /* GICD_TYPER */
	and x1, x1, #0x1f   /* extract ITLinesNumber - how many bitfields we need to set */
	add x3, x0, #0x80   /* GICD_IGROUPRn */
	mov w2, 0xffffffff  /* Set all interrupts to group 1 so they can be managed by non-secure code */
set_intr_group_1:
	str w2, [x3, x1, lsl #2]
	subs x1, x1, #1
	b.ge set_intr_group_1

	add x0, x0, #0x10000 /* GIC CPU interface base */
	mov w2, 0x1b /* Enable group 0 and group 1 interrupts, AckCtl, signal group 0 using FIQ */
	str w2, [x0] /* CTLR */
	mov w2, 0xff /* Set priority mask to minimum (allow all interrupts) */
	str w2, [x0, #4] /* PMR */
	mov w2, 0x2
	str w2, [x0, #8] /* BPR */

	mov x0, #0xff240000 /* IOU_SECURE_SLCR */
	ldr w2, =(0x04920492) /* Allow access to all peripherals from non-secure mode */
	str w2, [x0]
	str w2, [x0, #4]
	/* TODO: LPD_SLCR_SECURE configures some peripherals like ADMA and RPU to secure accesses by default
	 * FPD_SLCR_SECURE seems to configure all peripherals for non-secure access so this shouldn't be a problem
	 */

	/* EL1 is AArch64, disable SMC instruction, allow HVC instruction,
	 * disable exception routing to EL3 */
#if EL1_SECURE
	mov x2, #0x5b0
#else
	mov x2, #0x5b1
#endif
	msr scr_el3, x0

	mov x0, #0b1111000101 /* state EL1h, DAIF=1111, AArch64 */
	msr spsr_el3, x0
	adr x0, continue_el1
	msr elr_el3, x0
	eret

continue_el2:
	msr cptr_el2, xzr
	ldr x0, =0x30c00838
	msr sctlr_el2, x0
	/* Initialize to safe values */
	msr sctlr_el1, xzr
	msr vttbr_el2, xzr
	/* Allow write access to various registers from EL1 */
	mov x0, #0b1110011
	msr actlr_el2, x0
	mov x0, #((1 << 31) | (1 << 29)) /* EL1 Execution state is AArch64, disable HVC instruction */
	msr hcr_el2, x0
	mov x0, #0b1111000101 /* state EL1h, DAIF=1111, AArch64 */
	msr spsr_el2, x0
	adr x0, continue_el1
	msr elr_el2, x0
	eret

continue_el1:
	/* Interesting bits:
		EE == 0, E0E == 0 => little-endian at EL1 and EL0
		WXN == 0 => write permission doesn't imply execute never
		nTWE == 0, nTWI == 0 => trap WFE/WFI instructions at EL0
		UMA == 0 => trap accesses to DAIF system reg. from EL0
		SED == 0 => allow SETEND instruction in AArch32
		SA0 == 1, SA == 1 => alignment checks for SP accesses
		A == 0 => disable data alignment checks
		C, I, M == 0 => disable D-cache, I-cache, MMU (for now)
		TODO: we may want to set the below to 1, there are some userspace uses for them
		UCI == 0 => trap cache maintenance instructions executed at EL0
		UCT == 0 => trap accesses to CTR_EL0 (cache type reg.) from EL0
		DZE == 0 => trap DC ZVA instructions from EL0
	 */
	ldr x0, =0x30c00838
	msr sctlr_el1, x0

	/* TODO: interrupt handling in PLO doesn't save FP/SIMD registers, maybe enable this? */
	/* Don't trap floating point access */
	mrs x0, cpacr_el1
	orr x0, x0, #(0x3 << 20)
	msr cpacr_el1, x0
	isb

	/* Set vector table */
	adr	x0, _vector_table
	msr	vbar_el1, x0

	/* Invalidate caches and TLB */
	ic iallu
	bl invalidate_dcache
	tlbi vmalle1

	dsb sy
	isb

	/* Enable performance monitors */
	mrs x0, pmcr_el0
	orr x0, x0, #0x7 /* reset cycle counter, reset event counter, enable */
	orr x0, x0, #0x40 /* activate 64-bit counting mode */
	msr pmcr_el0, x0
	mrs x0, pmcntenset_el0
	orr x0, x0, #(1 << 31)
	msr pmcntenset_el0, x0

#ifdef __TARGET_AARCH64A53
	/* Enable SMP */
	mrs x0, s3_1_c15_c2_1 /* CPUECTLR_EL1 */
	orr x0, x0, #(0x1 << 6)
	msr s3_1_c15_c2_1, x0

	/* L1 Data prefetch control - 5, Enable device split throttle, 2 independent data prefetch streams */
	/* Set ENDCCASCI bit in CPUACTLR_EL1 register - needed due to Cortex-A53 erratum #855873 */
	ldr	x0, =0x1000080CA000
	msr	s3_1_c15_c2_0, x0 /* CPUACTLR_EL1 */
#endif

	adr	x1, _stack
	mov sp, x1

	/* Trap all cores except core 0 */
	adr x1, hal_coreJumpFlag
	str xzr, [x1]
	mrs x0, mpidr_el1
	and x0, x0, #0xff
	cbz x0, start_core_0
other_core_trap:
	dsb ish
	wfe
	ldr x0, [x1]
	cbz x0, other_core_trap
	/* Freed from trap, jump to kernel */
	b hal_exitToEL1

start_core_0:
	adr x1, _startc
	br x1


error_loop:
	b .

invalidate_dcache:
	/* 5.3.1 Cleaning and invalidating the caches */
	mov x0, #0x0             /* x0 = Cache level */
	msr csselr_el1, x0       /* 0x0 for L1 Dcache 0x2 for L2 Dcache. */
	mrs x4, ccsidr_el1       /* Read Cache Size ID. */
	and x1, x4, #0x7
	add x1, x1, #0x4         /* x1 = Cache Line Size. */
	ldr x3, =0x7fff
	and x2, x3, x4, lsr #13  /* x2 = Cache Set Number – 1. */
	ldr x3, =0x3ff
	and x3, x3, x4, lsr #3   /* x3 = Cache Associativity Number – 1. */
	clz w4, w3               /* x4 = way position in the CISW instruction. */
	mov x5, #0               /* x5 = way counter way_loop. */
way_loop:
	mov x6, #0               /* x6 = set counter set_loop. */
set_loop:
	lsl x7, x5, x4
	orr x7, x0, x7           /* Set way. */
	lsl x8, x6, x1
	orr x7, x7, x8           /* Set set. */
	dc isw, x7               /* Invalidate cache line. */
	add x6, x6, #1           /* Increment set counter. */
	cmp x6, x2               /* Last set reached yet? */
	ble set_loop             /* If not, iterate set_loop, */
	add x5, x5, #1           /* else, next way. */
	cmp x5, x3               /* Last way reached yet? */
	ble way_loop             /* If not, iterate way_loop. */
	ret

.size _start, .-_start
.ltorg


.align 4
.globl hal_exitToEL1
.type hal_exitToEL1, %function
hal_exitToEL1:
	dsb sy
	isb
	adr x0, hal_common
	ldp x9, x0, [x0] /* load syspage pointer, entry address */
	msr elr_el1, x0
	mov x0, #0b1111000101 /* state EL1h, DAIF=1111, AArch64 */
	msr spsr_el1, x0
	eret
.size hal_exitToEL1, .-hal_exitToEL1


/* Put interrupts and exceptions code before the vector table to avoid wasting
 * memory on padding (vector table requires 0x800 bytes alignment) */
#include "../_interrupts.S"
#include "../_exceptions.S"

.align 11
/* Vector Table Definition */
.globl _vector_table
_vector_table:
/* from EL1 with SP_EL0 */
.org(_vector_table)
	b .

.org (_vector_table + 0x80)
	b .

.org (_vector_table + 0x100)
	b .

.org (_vector_table + 0x180)
	b .

/* from EL1 with SP_EL1 */
.org (_vector_table + 0x200)
_synchronous_vector:
	b _exceptions_dispatch

.org (_vector_table + 0x280)
_irq_vector:
	b _interrupts_dispatch

.org (_vector_table + 0x300)
_fiq_vector:
	b _interrupts_dispatch

.org (_vector_table + 0x380)
_serror_vector:
	b _exceptions_dispatch

/* from EL0 in AArch64 */
.org (_vector_table + 0x400)
	b .

.org (_vector_table + 0x480)
	b .

.org (_vector_table + 0x500)
	b .

.org (_vector_table + 0x580)
	b .

/* from EL0 in AArch32 */
.org (_vector_table + 0x600)
	b .

.org (_vector_table + 0x680)
	b .

.org (_vector_table + 0x700)
	b .

.org (_vector_table + 0x780)
	b .

.org (_vector_table + 0x800)
.ltorg
