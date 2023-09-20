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
.size _init, . - _init
