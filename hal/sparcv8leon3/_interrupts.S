/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupts handlers for Leon3
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include <config.h>
#include "cpu.h"
#include "gaisler/grlib-tn-0018.h"

.section ".text"
.align 4
.global _interrupts_dispatch
.type _interrupts_dispatch, #function

/*
 * on entry:
 * %l0: psr
 * %l1: pc
 * %l2: npc
 * %l3: irq number
 */
_interrupts_dispatch:
	/* %g2, g3 used during manual window overflow */
	mov %g2, %l4
	mov %g3, %l5

	mov %wim, %g2
	/* check if we've just overflowed
	 * window overflow if wim == (1 << CWP)
	 * wim >> l0[4:0] - shift wim by CWP (lowest 5 bits from psr)
	 */
	srl %g2, %l0, %g3
	cmp %g3, 1
	/* calculate new wim: current %wim in %g2, %g3 is scratch */
	sll %g2, (NWINDOWS - 1), %g3

	bne wovfl_done
	srl %g2, 1, %g2

	save
	wr %g2, %g3, %wim
	nop
	nop
	nop
	std %l0, [%sp + 0x00]
	std %l2, [%sp + 0x08]
	std %l4, [%sp + 0x10]
	std %l6, [%sp + 0x18]
	std %i0, [%sp + 0x20]
	std %i2, [%sp + 0x28]
	std %i4, [%sp + 0x30]
	std %i6, [%sp + 0x38]
	restore

wovfl_done:
	/* save context on stack:
	 * %l0-%l2, %g1, %g2 (in %l4), %g3 (in %l5) %g4, %g5, %i0-%i7, %y
	 */

	sub %fp, 0x48, %sp

	std %l0, [%sp + 0x00] /* psr, PC */
	st  %l2, [%sp + 0x08] /* nPC */
	st  %g1, [%sp + 0x0c] /* g1 */
	std %l4, [%sp + 0x10] /* g2, g3 */
	std %g4, [%sp + 0x18] /* g4, g5 */
	std %i0, [%sp + 0x20] /* i0, i1 */
	std %i2, [%sp + 0x28] /* i2, i3 */
	std %i4, [%sp + 0x30] /* i4, i5 */
	std %i6, [%sp + 0x38] /* i6, i7 */
	rd  %y, %g1
	st  %g1, [%sp + 0x40] /* y */
	/* ends at %sp + 0x44 */

	/* allocate new stack frame for interrupt */
	mov %sp, %fp
	sub %sp, 0x60, %sp

	/* Enable traps, disable interrupts */
	or %l0, PSR_PIL, %l0
	wr %l0, PSR_ET, %psr
	nop
	nop
	nop

	/* int interrupts_dispatch(unsigned int irq) */
	call interrupts_dispatch
	mov %l3, %o0

	/* Disable traps, we can't let a trap use the free stack space */
#ifndef __CPU_GR712RC
	pwr 0, %psr
#else
	wr %l0, %psr
#endif
	nop
	nop
	nop

	/* back to task context */
	mov %fp, %sp

	ld  [%sp + 0x40], %g1 /* y */
	add %sp, 0x48, %fp
	wr  %g1, %g0, %y
	ldd [%sp + 0x38], %i6  /* i6, i7 */
	ldd [%sp + 0x30], %i4  /* i4, i5 */
	ldd [%sp + 0x28], %i2  /* i2, i3 */
	ldd [%sp + 0x20], %i0  /* i0, i1 */
	ldd [%sp + 0x18], %g4  /* g4, g5 */
	ldd [%sp + 0x10], %g2  /* g2, g3 */
	ld  [%sp + 0x0c], %g1  /* g1 */
	ldd [%sp + 0x00], %l0  /* psr, PC */
	ld  [%sp + 0x08], %l2  /* nPC */

	/* Check if restore would cause window underflow.
	 * After restore: CWP = CWP + 1 (mod NWINDOWS)
	 * i.e. wim >> (CWP + 1) == 1
	 */
	and	%l0, PSR_CWP, %l5
	add %l5, 1, %l5
	cmp %l5, NWINDOWS
	bne cwp_done
	rd %wim, %l4
	/* we'd end up in non-existent window #31, it means it's #0 */
	mov 0, %l5

cwp_done:
	/* l4 = wim, l5 = CWP + 1 (mod NWINDOWS)
	 * check if wim >> (CWP + 1) == 1 (window underflow)
	 */
	srl %l4, %l5, %l6
	cmp %l6, 1
	/* calculate new wim
	 * %l4 = current wim
	 * wim = (wim << 1) ^ (wim >> (NWINDOWS - 1))
	 */
	sll %l4, 1, %l5

	bne wufl_done
	srl %l4, (NWINDOWS - 1), %l4

	wr %l4, %l5, %wim
	nop
	nop
	nop
	restore
	ldd [%sp + 0x00], %l0
	ldd [%sp + 0x08], %l2
	ldd [%sp + 0x10], %l4
	ldd [%sp + 0x18], %l6
	ldd [%sp + 0x20], %i0
	ldd [%sp + 0x28], %i2
	ldd [%sp + 0x30], %i4
	ldd [%sp + 0x38], %i6
	save

wufl_done:

	TN_0018_WAIT_ICACHE(%l3, %l4)

	/* restore %psr */
	wr %l0, %psr
	nop
	nop
	nop

	TN_0018_FIX(%l3, %l4)

	jmp %l1
	rett %l2

.size _interrupts_dispatch, . - _interrupts_dispatch
