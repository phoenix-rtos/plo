/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low-level routines
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

#ifndef _LOW_H_
#define _LOW_H_

#include "types.h"

#define min(a, b)   ((a > b) ? b : a)

extern char _welcome_str[];
extern u16 _plo_timeout;
extern char _plo_command[];


typedef struct {
	u32 addr_lo;
	u32 addr_hi;
	u32 len_lo;
	u32 len_hi;
	u32 attr;
} low_mmitem_t;


#pragma pack(push, 1)

enum { mAttrRead = 0x01, mAttrWrite = 0x02, maAttrExec = 0x04, mAttrShareable = 0x08,
	   mAttrCacheable = 0x10, mAttrBufferable = 0x20, /* TODO: */ };


typedef struct _syspage_map_t {
	u32 start;
	u32 end;
	u32 attr;

	u8 id;
	char name[8];
} syspage_map_t;


typedef struct syspage_program_t {
	u32 start;
	u32 end;

	u8 dmap;
	u8 imap;

	char cmdline[16];
} syspage_program_t;


typedef struct _syspage_t {
	struct {
		void *text;
		u32 textsz;

		void *data;
		u32 datasz;

		void *bss;
		u32 bsssz;
	} kernel;

	u32 syspagesz;

	char *arg;

	u32 progssz;
	syspage_program_t *progs;

	u32 mapssz;
	syspage_map_t *maps;
} syspage_t;


#pragma pack(pop)


syspage_t *plo_syspage;
extern u32 kernel_entry;


/* Initialization functions */

extern void low_init(void);

extern void low_done(void);


/* Functions modify registers */

extern u8 low_inb(u16 addr);

extern void low_outb(u16 addr, u8 b);

extern u32 low_ind(u16 addr);

extern void low_outd(u16 addr, u32 d);



/* Functions make operations on memory */

extern void low_setfar(u16 segm, u16 offs, u16 v);

extern u16 low_getfar(u16 segm, u16 offs);

extern void low_setfarbabs(u32 addr, u8 v);

extern u8 low_getfarbabs(u32 addr);

extern void low_setfarabs(u32 addr, u32 v);

extern u32 low_getfarabs(u32 addr);

extern void low_copyto(u16 segm, u16 offs, void *src, unsigned int l);

extern void low_copyfrom(u16 segm, u16 offs, void *dst, unsigned int l);

extern void low_memcpy(void *dst, const void *src, unsigned int l);

extern void low_copytoabs(u32 addr, void *src, unsigned int l);

extern void low_copyfromabs(u32 addr, void *dst, unsigned int l);

extern u16 low_getcs(void);

/* Function prepares memory map for kernel */
extern int low_mmcreate(void);

/* Function returns selected memory map item */
extern int low_mmget(unsigned int n, low_mmitem_t *mmitem);

/* Function starts kernel loaded into memory */
extern int low_launch(void);



/* Opeartions on interrupts */
extern void low_cli(void);

extern void low_sti(void);

extern void low_maskirq(u16 n, u8 v);

extern int low_irqinst(u16 irq, int (*isr)(u16, void *), void *data);

extern int low_irquninst(u16 irq);



/* Communication functions */
extern void low_setattr(char attr);

extern void low_putc(char c);

extern void low_getc(char *c, char *sc);

extern int low_keypressed(void);


#endif
