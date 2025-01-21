/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low level initialization
 *
 * Copyright 2022-2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

 #define __ASSEMBLY__

#include <board_config.h>
#include "config.h"
#include "hal/sparcv8leon/cpu.h"

/* SDRAM registers */
#define SDCFG1   0xffe00000
#define SDCFG2   0xffe00004
#define SDMUXCFG 0xffe00020

/* PROM registers */
#define PROM_MCFG1 0xff903000
#define PROM_MCFG3 0xff903008
#define PROM_MCFG5 0xff903010
#define PROM_MCFG7 0xff903018

/* L2 Cache registers */
#define L2CC     0xf0000000
#define L2CS     0xf0000004
#define L2CFMA   0xf0000008
#define L2CERR   0xf0000020
#define L2CACCC  0xf000003c

.extern syspage_common


.section ".init", "ax"
.align 4
.global _init
.type _init, #function
_init:
	wr %g0, %wim
	nop
	nop
	nop

	wr %g0, PSR_S, %psr

	/* Get CPU ID */
	rd %asr17, %g1
	srl %g1, 28, %g1
	cmp %g1, %g0
	bnz jmp_core
	nop

	/* Flush L1 cache */
	set 0x600000, %g1
	sta %g1, [%g0] ASI_CCTRL

	set ((1 << 15) | (1 << 14)), %g2

wait_flush:
	lda [%g0] ASI_CCTRL, %g1
	andcc %g1, %g2, %g0
	bnz wait_flush
	nop

	/* Enable snooping, D & I cache */
	set 0x80000f, %g1
	sta %g1, [%g0] ASI_CCTRL

	/* Configure PROM */
	set PROM_MCFG1, %g1
	set MCFG1_VAL, %g2
	st %g2, [%g1]

	set PROM_MCFG3, %g1
	set MCFG3_VAL, %g2
	st %g2, [%g1]

	set PROM_MCFG5, %g1
	set MCFG5_VAL, %g2
	st %g2, [%g1]

	set PROM_MCFG7, %g1
	set MCFG7_VAL, %g2
	st %g2, [%g1]

#ifndef ram
	/* Configure SDRAM */
	set SDCFG1, %g1
	set SDCFG2, %g2

	/* Disable SDRAM controller */
	st %g0, [%g1]
	/* Configure SDCFG2 */
	set ((1 << 30) | (SDCFG2_EN2T << 15) | (SDCFG2_DCS << 14) | (SDCFG2_BPARK << 13)), %g3
	st %g3, [%g2]
	/* Set parameters and enable controller */
	set ((1 << 31) | (SDCFG1_TRP << 30) | (SDCFG1_TRFC << 27) | (SDCFG1_TCAS << 26) | (SDCFG1_BANKSZ << 23) | (SDCFG1_COLSZ << 21) | (SDCFG1_RFLOAD << 0)), %g3
	st %g3, [%g1]

	/* Disable ECC (not supported on GR740-MINI) */
	set SDMUXCFG, %g1
	st %g0, [%g1]
#endif

	/* Disable and invalidate L2 cache */
	set L2CFMA, %g1
	set ((1 << 3) | 5), %g2
	st %g2, [%g1]

	/* Wait for initializations to complete:
	 * SDRAM initialization takes a max of 100 SDRAM clock cycles.
	 * Full L2 cache invalidation takes 5 cycles for the 1st line
	 * and 1 cycle for each subsequent line.
	 * There are 0x8000 lines.
	 */

	set 0x2001, %g1
1:
	nop
	subcc %g1, 1, %g1
	bne 1b
	nop

	/* Initialize cache according to GRLIB-TN-0021 errata */

	/* Set L2CERR to default (0) */
	set L2CERR, %g1
	st %g0, [%g1]

	/* Set L2CACCC */
	set L2CACCC, %g1
	set ((1 << 14) | (1 << 13) | (1 << 10) | (1 << 4) | (1 << 2) | (1 << 1)), %g2
	st %g2, [%g1]

	/* Enable cache with default params, EDAC disabled, LRU */
	set L2CC, %g1
	set (1 << 31), %g2
	st %g2, [%g1]

	stbar

	/* Perform load from cacheable memory (errata) */
	ld [%g0], %g0

	/* Set up trap table */
	sethi %hi(_trap_table), %g1
	wr %g1, %tbr

	wr %g0, 0x2, %wim

	clr %fp

	/* Stack pointer */
	set _stack, %sp
	sub %sp, 0x60, %sp

	/* Set PSR to supervisor, enable traps, disable irq */
	wr %g0, (PSR_ET | PSR_S | PSR_PIL), %psr
	nop
	nop
	nop

	wr %g0, 0x2, %wim

	sethi %hi(_startc), %g1
	jmpl %g1 + %lo(_startc), %g0
	clr %g1

jmp_core:
	set syspage_common, %g1
	ld [%g1], %g1 /* syspage_common->syspage */
	ld [%g1 + 8], %g1 /* syspage->pkernel */

	/* Jump to kernel */
	jmp %g1
	nop
.size _init, . - _init
