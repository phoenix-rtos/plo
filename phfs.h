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

#include "types.h"


extern s32 phfs_open(u16 dn, char *name, u32 flags);

extern s32 phfs_read(u16 dn, s32 handle, u32 *pos, u8 *buff, u32 len);

extern s32 phfs_write(u16 dn, s32 handle, u32 *pos, u8 *buff, u32 len, u8 sync);

extern s32 phfs_close(u16 dn, s32 handle);

extern void phfs_init(void);


#endif
