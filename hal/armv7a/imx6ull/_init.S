/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * Low-level initialization for iMX6ULL processor
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

#define ADDR_AIPSTZ 0x0207c000
#define ADDR_CCM    0x020c4000

.arm

.section .init, "ax"

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


.org 0x400, 0x0
#ifdef PLO_NOR_BOOT
qspi_config:
	.word 0          /* DQS Loopback */
	.word 0          /* Hold delay */
	.word 0, 0       /* Reserved */
	.word 0          /* Quad mode enable */
	.word 0          /* device_cmd */
	.word 0          /* write_cmd_ipcr */
	.word 0x02000000 /* write_enable_ipcr */
	.word 3          /* Chip select hold time */
	.word 3          /* Chip select setup time */
	.word 0x200000   /* Serial flash A1 size */
	.word 0          /* Serial flash A2 size */
	.word 0          /* Serial flash B1 size */
	.word 0          /* Serial flash B2 size */
	.word 0          /* Serial clock freq (18 MHz) */
	.word 0          /* busy_bit_offset */
	.word 1          /* Mode of operation of serial flash (Single pad) */
	.word 0          /* Serial Flash Port B Selection */
	.word 0          /* Dual Data Rate mode enable */
	.word 0          /* Data Strobe Signal enable in Serial Flash */
	.word 0          /* Parallel Mode */
	.word 0          /* CS1 on Port A */
	.word 0          /* CS1 on Port B */
	.word 0          /* Full Speed Phase Selection */
	.word 0          /* Full Speed Delay Selection */
	.word 0          /* DDR Sampling Point */
	/* LUT program sequance */
	.word 0x08180403, 0x24001C00, 0x00000000, 0x00000000 /* Standard READ 0x3 command. */
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
	.word 0x00000000, 0x00000000, 0x00000000, 0x00000000

	.word 0          /* read_status_ipcr */
	.word 0          /* enable_dqs_phase */
	/* Reserved 36 bytes */
	.word 0, 0, 0, 0
	.word 0, 0, 0, 0
	.word 0

	.word 0          /* dqs_pad_setting_override */
	.word 0          /* sclk_pad_setting_override */
	.word 0          /* data_pad_setting_override */
	.word 0          /* cs_pad_setting_override */
	.word 0          /* dqs_loopback_internal */
	.word 0          /* dqs_phase_sel */
	.word 0          /* dqs_fa_delay_chain_sel */
	.word 0          /* dqs_fb_delay_chain_sel */
	.word 0          /* sclk_fa_delay_chain_sel */
	.word 0          /* sclk_fb_delay_chain_sel */
	/* Reserved 64 bytes */
	.word 0, 0, 0, 0
	.word 0, 0, 0, 0
	.word 0, 0, 0, 0
	.word 0, 0, 0, 0

	.word 0xc0ffee01 /* tag */


.org 0x1000, 0x0
#endif
ivt:
	.word 0x402000d1 /* hdr */
	.word _start     /* entry */
	.word 0          /* reserved 1 */
	.word 0          /* dcd */
	.word boot_data  /* boot_data */
	.word ivt        /* self */
	.word 0          /* csf */
	.word 0          /* reserved 2 */

boot_data:
	.word _plo_load_addr /* load address */
	.word _plo_size      /* size - will be changed during image creation */
	.word 0              /* plugin */
	.word 0


_start:
	cpsid aif, #SYS_MODE

	/* Only CPU0 performs initialization, others go into wfi */
	mrc p15, 0, r1, c0, c0, 5 /* Read Multiprocessor Affinity Register */
	and r1, r1, #0xf          /* Extract CPU ID                        */
	cmp r1, #0
	beq initialize


/* TODO: make appropriate action when other core than CPU_0 is running */
wait_loop:
	wfi
	b wait_loop


initialize:
	/* Initialize AIPSTZ 1,2,3 clocks */
	ldr r0, =ADDR_CCM
	ldr r1, [r0, #0x68] /* CCM CGR_0 */
	orr r1, r1, #0x5
	str r1, [r0, #0x68]

	ldr r0, =ADDR_CCM
	ldr r1, [r0, #0x80] /* CCM CGR_6 */
	orr r1, r1, #0x40000
	str r1, [r0, #0x80]

	mov r2, #3

	/* AIPS bridge should be programmed before usage */
	ldr r0, =ADDR_AIPSTZ
	add r0, #0x40
	mov r1, #0
aipstzl:
	str r1, [r0]
	str r1, [r0, #4]
	str r1, [r0, #8]
	str r1, [r0, #12]
	str r1, [r0, #16]
	add r0, r0, #0x00100000
	subs r2, r2, #1
	bne aipstzl

	/* Enable PMU */
	mrc p15, 0, r0, c9, c12, 0       /* Read PMCR (Performance Monitor Control Register)  */
	orr r0, #0x7                     /* Cycle counter reset - bit[2], Performance counter reset - bit[1], enable all counters - bit[0] */
	mcr p15, 0, r0, c9, c12, 0       /* Write PMCR (Performance Monitor Control Register) */
	mrc p15, 0, r0, c9, c12, 1       /* Read CESR (Count Enable Set Register)             */
	orr r0, #1 << 31                 /* Enable cycle counter                              */
	mcr p15, 0, r0, c9, c12, 1       /* Write CESR (Count Enable Set Register)            */


	/* Disable MMU */
	mrc p15, 0, r1, c1, c0, 0        /* Read SCTLR (System Control Register) data  */
	bic r1, r1, #0x1                 /* Disable MMU                                */
	mcr p15, 0, r1, c1, c0, 0        /* Write SCTLR (System Control Register) data */


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
	ldr r0, =_vector_table
	mcr p15, 0, r0, c12, c0, 0       /* Write to VBAR (Vector Base Address Register) */


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
	sub r0, r0, #0x200

	/* Supervisor mode stack */
	msr CPSR_c, #(SVC_MODE | NO_INT)
	mov sp, r0
	sub r0, r0, #0x100

	/* Undefined mode stack */
	msr CPSR_c, #(UND_MODE | NO_INT)
	mov sp, r0
	sub r0, r0, #0x20

	/* Abort mode stack */
	msr CPSR_c, #(ABT_MODE | NO_INT)
	mov sp, r0
	sub r0, r0, #0x100

	/* System mode stack */
	msr CPSR_c, #(SYS_MODE | NO_INT)
	mov sp, r0

	/* Jump to plo */
	ldr r8, =_startc
	bx r8
.size _start, .-_start


#include "../_interrupts.S"
#include "../_exceptions.S"
