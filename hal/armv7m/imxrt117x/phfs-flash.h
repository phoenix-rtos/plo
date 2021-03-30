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
#include "../phfs.h"


extern int phfsflash_init(void);

extern handle_t phfsflash_open(u16 fn, const char *name, u32 flags);

extern s32 phfsflash_write(u16 fn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync);

extern s32 phfsflash_read(u16 fn, handle_t handle, addr_t *pos, u8 *buff, u32 len);

extern s32 phfsflash_close(u16 fn, handle_t handle);


#endif
