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


/* Functions modify registers */

extern u8 hal_inb(u16 addr);

extern void hal_outb(u16 addr, u8 b);

extern u32 hal_ind(u16 addr);

extern void hal_outd(u16 addr, u32 d);


/* Functions make operations on memory */

extern void hal_setfar(u16 segm, u16 offs, u16 v);

extern u16 hal_getfar(u16 segm, u16 offs);

extern void hal_setfarbabs(u32 addr, u8 v);

extern u8 hal_getfarbabs(u32 addr);

extern void hal_setfarabs(u32 addr, u32 v);

extern u32 hal_getfarabs(u32 addr);

extern void hal_copyto(u16 segm, u16 offs, void *src, unsigned int l);

extern void hal_copyfrom(u16 segm, u16 offs, void *dst, unsigned int l);

extern void hal_memcpy(void *dst, const void *src, unsigned int l);

extern void hal_copytoabs(u32 addr, void *src, unsigned int l);

extern void hal_copyfromabs(u32 addr, void *dst, unsigned int l);

extern u16 hal_getcs(void);


/* Function prepares memory map for kernel */

extern int hal_mmcreate(void);


/* Function returns selected memory map item */

extern int hal_mmget(unsigned int n, low_mmitem_t *mmitem);


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
