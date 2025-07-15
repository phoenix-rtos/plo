/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low-level initialization for Cortex-R5 (ARMv7) architecture
 * for TDA4VM target
 *
 * Copyright 2024, 2025 Phoenix Systems
 * Author: Lukasz Leczkowski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "hal/armv7r/cpu.h"
#include "config.h"

#define RAT_BASE_ADDR      0x40F90000
#define RAT_REG_CTRL_n     0x20
#define RAT_REGION_ENABLE  (1 << 31)
#define RAT_REGION_SIZE_64 6


.section .init, "ax"
.org 0
/* Because this vector table will be re-mapped to a different address,
 * it needs to be position-independent */
.globl _vector_table
.type _vector_table, %object
_vector_table:
	ldr pc, [pc, #24]
	ldr pc, [pc, #24]
	ldr pc, [pc, #24]
	ldr pc, [pc, #24]
	ldr pc, [pc, #24]
	b .
	ldr pc, [pc, #24]
	ldr pc, [pc, #24]

.word _start
.word _exception_undef
.word _syscalls_dispatch
.word _exception_prefetch
.word _exception_abort
.word 0
.word _interrupts_dispatch
.word _interrupts_dispatch

.global _start
.type _start, %function
_start:
	mov r0, #0
	mov r1, #0
	mov r2, #0
	mov r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	mov r9, #0
	mov r10, #0
	mov r11, #0
	mov r12, #0
	mov r13, #0
	mov r14, #0

	/* Disable L1 caches and MPU */
	mrc p15, 0, r1, c1, c0, 0        /* Read SCTLR (System Control Register) data  */
	bic r1, r1, #(1 << 12)           /* Disable ICache                             */
	bic r1, r1, #(1 << 2)            /* Disable DCache                             */
	bic r1, r1, #(1 << 0)            /* Disable MPU                                */
	orr r1, r1, #(1 << 13)			 /* Enable Hivecs (vectors at 0xffff0000)      */
	mcr p15, 0, r1, c1, c0, 0        /* Write SCTLR (System Control Register) data */
	dsb

	/* Configure MPU - necessary on this platform due to its memory layout */
	mov r0, #0
	adr r4, _preinit_mpu_data
_load_mpu_loop:
	ldmia r4!, {r1, r2, r3}
	mcr p15, 0, r0, c6, c2, 0        /* Select region */
	mcr p15, 0, r1, c6, c1, 0        /* Write DRBAR */
	mcr p15, 0, r2, c6, c1, 4        /* Write DRACR */
	mcr p15, 0, r3, c6, c1, 2        /* Write DRSR */
	add r0, r0, #1
	cmp r0, #16
	blt _load_mpu_loop

	mrc p15, 0, r1, c1, c0, 0        /* Read SCTLR (System Control Register) data  */
	orr r1, r1, #(1 << 0)            /* Enable MPU                                 */
	mcr p15, 0, r1, c1, c0, 0        /* Write SCTLR (System Control Register) data */
	dsb

	mrc p15, 0, r1, c1, c0, 1	     /* Read ACTLR */
	orr r1, r1, #(1 << 25)           /* Enable ECC on ATCM */
	/* [5:3] = 0b000 - Generate abort on parity errors, force write-through, enable hardware recovery */
	bic r1, r1, #(1 << 5)
	bic r1, r1, #(1 << 4)
	bic r1, r1, #(1 << 3)
	/* Disable linefill optimization to work around erratum i2099 (see document SPRZ455F) */
	orr r1, r1, #(1 << 13)           /* Set DLFO bit */
	mcr p15, 0, r1, c1, c0, 1	     /* Write ACTLR */

	dsb
	mov r1, #1 					     /* Enable ATCM at address 0 */
	mcr p15, 0, r1, c9, c1, 1        /* Write ATCM Region Register */

	/* Use RAT to map our vector table where we need it */
	ldr r1, =#(RAT_BASE_ADDR + RAT_REG_CTRL_n + RAT_REGION_VECTORS_REMAP * 0x10) /* R5FSS_RAT_CTRL_n register */
	ldr r0, =0xffff0000 /* Where vectors should be with HIVECS */
	str r0, [r1, #4] /* Translation base */
	ldr r0, =_vector_table
	str r0, [r1, #8] /* Translated address low bits */
	mov r0, #0x0
	str r0, [r1, #12] /* Translated address high bits */
	ldr r0, =#(RAT_REGION_ENABLE | RAT_REGION_SIZE_64)
	str r0, [r1, #0] /* Size and enable */
	dsb

	/* Invalidate L1 ICache */
	mov r1, #0
	mcr p15, 0, r1, c7, c5, 0        /* Clear ICIALLU */
	mcr p15, 0, r1, c15, c5, 0
	dsb
	isb

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
	dsb
	isb

	/* Enable L1 Caches */
	mrc p15, 0, r1, c1, c0, 0         /* Read SCTLR (System Control Register) data  */
	orr r1, r1, #(1 << 2)             /* Enable data cache                          */
	orr r1, r1, #(1 << 12)            /* Enable instruction cache                   */
	mcr p15, 0, r1, c1, c0, 0         /* Write SCTLR (System Control Register) data */
	dsb
	isb

	/* Enable PMU */
	mrc p15, 0, r0, c9, c12, 0       /* Read PMCR (Performance Monitor Control Register)  */
	orr r0, #0x7                     /* Cycle counter reset - bit[2], Performance counter reset - bit[1], enable all counters - bit[0] */
	mcr p15, 0, r0, c9, c12, 0       /* Write PMCR (Performance Monitor Control Register) */
	mrc p15, 0, r0, c9, c12, 1       /* Read CESR (Count Enable Set Register)             */
	orr r0, #1 << 31                 /* Enable cycle counter                              */
	mcr p15, 0, r0, c9, c12, 1       /* Write CESR (Count Enable Set Register)            */

	/* Setup initial SP */
	ldr r0, =_stack
	bic r0, #7

	/* FIQ mode stack */
	msr CPSR_c, #(MODE_FIQ | NO_INT)
	mov sp, r0
	sub r0, r0, #0x20

	/* IRQ mode stack */
	msr CPSR_c, #(MODE_IRQ | NO_INT)
	mov sp, r0
	sub r0, r0, #0x100

	/* Supervisor mode stack */
	msr CPSR_c, #(MODE_SVC | NO_INT)
	mov sp, r0
	sub r0, r0, #0x40

	/* Undefined mode stack */
	msr CPSR_c, #(MODE_UND | NO_INT)
	mov sp, r0
	sub r0, r0, #0x40

	/* Abort mode stack */
	msr CPSR_c, #(MODE_ABT | NO_INT)
	mov sp, r0
	sub r0, r0, #0x40

	/* System mode stack */
	msr CPSR_c, #(MODE_SYS | NO_INT)
	mov sp, r0

	/* Jump to plo */
	ldr r8, =_startc
	bx r8
.size _start, .-_start
.ltorg

_preinit_mpu_data:
/* Shared DDR */
.word 0x80000000 /* Base address */
.word 0x30f /* Normal cacheable Write-Back, Write-Allocate, PL1: Read/Write PL0: Read/Write, Executable, Shareable */
.word ((30 << 1) | 1) /* size 2 GB, enabled */
/* Devices */
.word 0 /* Base address */
.word 0x1305 /* Device, PL1: Read/Write PL0: Read/Write, Execute never, Shareable */
.word ((30 << 1) | 1) /* size 2 GB, enabled */
/* Kernel text - ATCM+BTCM+MCU_MSRAM (mapped from 0x41cf0000) */
.word ADDR_ATCM /* Base address */
.word 0x102 /* Normal cacheable Write-Through, no Write-Allocate, PL1: Read/Write PL0: No access, Executable, Non-shareable */
.word ((16 << 1) | 1) /* size 128 KB, enabled */
/* User code/data - MCU_MSRAM */
.word ADDR_MSRAM /* Base address */
.word 0x302 /* Normal cacheable Write-Through, no Write-Allocate, PL1: Read/Write PL0: Read/Write, Executable, Non-shareable */
.word (19 << 1) | 1 /* Size 1M, enabled */
/* Block access to re-mapped part of MCU_MSRAM */
.word ORIG_MSRAM_REMAP /* Base address */
.word 0x0 /* No access */
.word ((15 << 1) | 1) /* size 64 KB, enabled */
/* Zero out remaining regions */
.rept 11
.word 0
.word 0
.word 0 /* Disabled */
.endr
