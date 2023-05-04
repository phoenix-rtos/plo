/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low level initialization
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "config.h"
#include "../cpu.h"

.extern _plo_load_addr
.extern _plo_size

.section ".init", "ax"
.align 4
.global _init
.type _init, #function
_init:
	/* get current PC */
	call . + 8
	nop
	mov %o7, %g5
	/* check where we're loaded */
	set _plo_load_addr, %g2
	cmp %g5, %g2
	bge stage1

	/* Enable CGU for SRAM */
	set 0x80006000, %g1 /* %g1 = cgu_unlock */
	add %g1, 0x8, %g3   /* %g3 = cgu_core_reset */
	add %g1, 0x4, %g4   /* %g4 = cgu_clk_en */
	ld [%g1], %g2
	or %g2, 0x10, %g2
	st %g2, [%g1]

	ld [%g3], %g2
	or %g2, 0x10, %g2
	st %g2, [%g3]

	ld [%g4], %g2
	or %g2, 0x10, %g2
	st %g2, [%g4]

	ld [%g4], %g2
	andn %g2, 0x10, %g2
	st %g2, [%g4]

	ld [%g3], %g2
	andn %g2, 0x10, %g2
	st %g2, [%g3]

	ld [%g4], %g2
	or %g2, 0x10, %g2
	st %g2, [%g4]

	ld [%g1], %g2
	andn %g2, 0x10, %g2
	st %g2, [%g1]

	/* Initialize SRAM pins */
	sethi %hi(0x8030C000), %g1
	sethi %hi(0xFE1FFFFF), %g3
	or %g3, %lo(0xFE1FFFFF), %g3
	st %g3, [%g1]

	sethi %hi(0x8030D000), %g1
	sethi %hi(0x1E0000), %g3
	or %g3, 7, %g3
	st %g3, [%g1]

	sethi %hi(0x8000D000), %g1
	sethi %hi(0x22222000), %g2
	or %g2, 0x222, %g4
	st %g4, [%g1]

	/* 0x8000D004 */
	add %g1, 0x4, %g1
	st %g4, [%g1]

	/* 0x8000D008 */
	add %g1, 0x4, %g1
	set 0x22222, %g3
	st %g3, [%g1]

	/* 0x8000D00C */
	add %g1, 0x4, %g1
	or %g2, %lo(0x22222220), %g3
	st %g3, [%g1]

	/* 0x8000D010 */
	add %g1, 0x4, %g1
	mov 0x222, %g3
	st %g3, [%g1]

	/* 0x8000D018 */
	add %g1, 0x8, %g1
	set 0x22220, %g3
	st %g3, [%g1]

	/* Set SRAM bank size and enable bus ready signalling */
	set 0x80000004, %g1
	set ((0xF << 9) | (1 << 7)), %g2
	st %g2, [%g1]

	/* Copy plo to SRAM */
	set _plo_size, %g1
	/* src = %g5 - 0x1000 */
	set 0x1000, %g3
	sub %g5, %g3, %g2       /* src */
	set _plo_load_addr, %g3 /* dst */
	add %g3, %g1, %g4       /* end */

copy:
	ld [%g2], %g5
	add %g2, 4, %g2
	st %g5, [%g3]
	add %g3, 4, %g3
	cmp %g3, %g4
	bl copy
	nop

stage1:
	/* Set up trap table */
	sethi %hi(_trap_table), %g1
	wr %g1, %tbr

	wr %g0, 0x2, %wim

	clr %fp

	/* Stack pointer */
	set _stack, %sp
	sub %sp, 0x60, %sp

	/* Set PSR to supervisor, enable traps */
	mov %psr, %g1
	or %g1, (PSR_ET | PSR_S), %g1
	wr %g1, %psr

	sethi %hi(_startc), %g1
	jmpl %g1 + %lo(_startc), %g0
	mov %g0, %g1
