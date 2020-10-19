/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * PHoenix FileSystem
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PHFS_H_
#define _PHFS_H_

#include "config.h"

typedef struct {
	s32 h;
	void *offs;
} handle_t;


typedef struct {
	handle_t (*open)(u16, const char *, u32);
	s32 (*read)(u16, handle_t, addr_t *, u8 *, u32);
	s32 (*write)(u16, handle_t, addr_t *, u8 *, u32, u8);
	s32 (*close)(u16, handle_t);
	unsigned int dn;
} phfs_handler_t;


extern handle_t phfs_open(u16 dn, const char *name, u32 flags);

extern s32 phfs_read(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len);

extern s32 phfs_write(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync);

extern s32 phfs_close(u16 dn, handle_t handle);

extern void phfs_init(void);


#endif
