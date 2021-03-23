/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * Interface for phfs serial
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PHFS_SERIAL_H_
#define _PHFS_SERIAL_H_

#include "../types.h"
#include "../phfs.h"


extern int phfs_serialInit(void);


extern void phfs_serialDeinit(void);


extern handle_t phfs_serialOpen(u16 dn, const char *name, u32 flags);


extern s32 phfs_serialWrite(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync);


extern s32 phfs_serialRead(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len);


extern s32 phfs_serialClose(u16 dn, handle_t handle);


#endif
