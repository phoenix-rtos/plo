/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2012, 2017, 2020 Phoenix Systems
 * Copyright 2006 Radoslaw F. Wawrzusiak
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Authors: Pawel Pisarczyk, Radoslaw F. Wawrzusiak, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_H_
#define _HAL_H_

#include "cmd.h"
#include "phfs.h"
#include "types.h"

#define min(a, b)   ((a > b) ? b : a)


typedef struct {
	u32 addr_lo;
	u32 addr_hi;
	u32 len_lo;
	u32 len_hi;
	u32 attr;
} low_mmitem_t;



/* Initialization functions */

extern void hal_init(void);

extern void hal_done(void);

extern void hal_initphfs(phfs_handler_t *handlers);

extern void hal_initdevs(cmd_device_t **devs);

extern void hal_appendcmds(cmd_t *cmds);


/* Setters and getters for common data */

extern void hal_setDefaultIMAP(char *imap);

extern void hal_setDefaultDMAP(char *dmap);

extern void hal_setKernelEntry(u32 addr);

extern void hal_setLaunchTimeout(u32 timeout);

extern u32 hal_getLaunchTimeout(void);

extern addr_t hal_vm2phym(addr_t addr);


/* Functions make operations on memory */

extern void hal_memcpy(void *dst, const void *src, unsigned int l);

extern void hal_memset(void *dst, int v, unsigned int l);


/* Function starts kernel loaded into memory */

extern int hal_launch(void);


/* Opeartions on interrupts */

extern void hal_cli(void);

extern void hal_sti(void);

extern void hal_maskirq(u16 n, u8 v);

extern int hal_irqinst(u16 irq, int (*isr)(u16, void *), void *data);

extern int hal_irquninst(u16 irq);


/* Communication functions */

extern void hal_setattr(char attr);

extern void hal_putc(char c);

extern void hal_getc(char *c, char *sc);

extern int hal_keypressed(void);


#endif
