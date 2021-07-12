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
#include "string.h"
#include "types.h"

/* Boot device number */
extern unsigned char _plo_bdn;

/* TODO: temporary HAL syspage, should be replaced by new syspage implementation */
/* Loader syspage */
extern syspage_hal_t _plo_syspage;

/* Executes BIOS interrupt calls */
extern void _interrupts_bios(unsigned char intr, unsigned short ds, unsigned short es);

#endif


/* Platform specific definitions */
#define SIZE_PAGE 0x1000


/* Kernel path */
#define PATH_KERNEL "phoenix-ia32-generic.elf"


/* Low memory layout */
#define ADDR_GDT 0x1000
#define SIZE_GDT 0x800

#define ADDR_IDT 0x1800
#define SIZE_IDT 0x800

/* FIXME: temporary HAL syspage, should be replaced by new syspage implementation */
#define ADDR_SYSPAGE 0x2000
#define SIZE_SYSPAGE 0x1000

#define ADDR_PDIR   0x3000
#define ADDR_PTABLE 0x4000

#define ADDR_STACK 0x6000
#define SIZE_STACK 0x1000

#define ADDR_PLO 0x7c00

/* Disk caches (below upper memory starting at 0x80000) */
#define ADDR_RCACHE 0x78000
#define SIZE_RCACHE 0x4000

#define ADDR_WCACHE 0x7c000
#define SIZE_WCACHE 0x4000


#endif
