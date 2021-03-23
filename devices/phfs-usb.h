/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * Interface for phfs USB
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PHFS_USB_H_
#define _PHFS_USB_H_

#include "../types.h"
#include "../phfs.h"


extern void phfs_usbInit(void);


extern void phfs_usbDeinit(void);


extern handle_t phfs_usbOpen(u16 dn, const char *name, u32 flags);


extern s32 phfs_usbWrite(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync);


extern s32 phfs_usbRead(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len);


extern s32 phfs_usbClose(u16 dn, handle_t handle);


#endif
