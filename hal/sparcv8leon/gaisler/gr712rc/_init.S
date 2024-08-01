/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low level initialization
 *
 * Copyright 2022-2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

 #define __ASSEMBLY__

#include "config.h"
#include "../../cpu.h"


.extern syspage_common


.section ".init", "ax"
.align 4
.global _init
.type _init, #function
_init:
	/* Get current PC
	 * Note: call is PC-relative - no need to worry about relocation
	 */
	call . + 8
	nop
	mov %o7, %g5

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

	set _plo_load_addr, %g3
	cmp %g5, %g3
	bge stage1
	nop

	/* We're executing in FLASH, copy PLO to RAM */
	set _plo_size, %g1
	/* src = %g5 - 0x1000 */
	set 0x1000, %g2
	sub %g5, %g2, %g2 /* src */
	/* dst = _plo_load_addr (%g3) */
	add %g3, %g1, %g4 /* end */

copy:
	ld [%g2], %g5
	add %g2, 4, %g2
	st %g5, [%g3]
	add %g3, 4, %g3
	cmp %g3, %g4
	bl copy
	nop

	/* Jump to RAM */
	sethi %hi(stage1), %g1
	jmp %g1 + %lo(stage1)
	nop

stage1:
	/* Set up trap table */
	sethi %hi(_trap_table), %g1
	wr %g1, %tbr

	wr %g0, 0x2, %wim

	/* Flush and enable cache - section 4.5.7 GR712RC UM */
	flush
	set 0x81000f, %g1
	sta %g1, [%g0] ASI_CCTRL

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
