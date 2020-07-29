/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MX RT Flash manager
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _FLASH_H_
#define _FLASH_H_

#include "../types.h"


extern s32 flash_open(u16 fn, char *name, u32 flags);

extern s32 flash_write(u16 fn, s32 handle, u32 *pos, u8 *buff, u32 len, u8 sync);

extern s32 flash_read(u16 fn, s32 handle, u32 *pos, u8 *buff, u32 len);

extern s32 flash_close(u16 fn, s16 handle);


#endif
