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

.section ".init", "ax"
.align 4
.global _init
.type _init, #function
_init:
	/* Set up trap table */
	set _trap_table, %g1
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
	nop
	nop
	nop

	call _startc
	mov %g0, %g1
