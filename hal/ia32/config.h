/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration
 *
 * Copyright 2001, 2005, 2006 Pawel Pisarczyk
 * Copyright 2012, 2020, 2021 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __ASSEMBLY__

#include "cpu.h"
#include "peripherals.h"
#include "types.h"

#include <phoenix/arch/ia32/syspage.h>
#include <phoenix/syspage.h>


/* Executes BIOS interrupt calls */
extern void _interrupts_bios(unsigned char intr, unsigned short ds, unsigned short es);

#endif


/* Kernel path */
#define PATH_KERNEL "phoenix-ia32-generic.elf"


/* Platform can set graphic modes (i.e. via VBE) */
#define HAS_GRAPHICS 1


/* Import platform specific definitions */
#include "ld/ia32-generic.ldt"

#endif
