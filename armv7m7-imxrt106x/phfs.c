/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Phoenix FileSystem
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../errors.h"
#include "../low.h"
#include "../plostd.h"
#include "../phfs.h"

#include "config.h"
#include "flash.h"

#define TEMP 2

struct {
    s32 (*open)(u16, char *, u32);
    s32 (*read)(u16, s32, u32 *, u8 *, u32);
    s32 (*write)(u16, s32, u32 *, u8 *, u32, u8);
    s32 (*close)(u16, s16);
    unsigned int dn;
} phfs_handlers[PDN_NB];



s32 phfs_open(u16 pdn, char *name, u32 flags)
{
    if (pdn > PDN_NB)
        return ERR_ARG;

    return phfs_handlers[pdn].open(phfs_handlers[pdn].dn, name, flags);
}


s32 phfs_read(u16 pdn, s32 handle, u32 *pos, u8 *buff, u32 len)
{
    if (pdn > PDN_NB)
        return ERR_ARG;

    return phfs_handlers[pdn].read(phfs_handlers[pdn].dn, handle, pos, buff, len);
}


s32 phfs_write(u16 pdn, s32 handle, u32 *pos, u8 *buff, u32 len, u8 sync)
{
    if (pdn > PDN_NB)
        return ERR_ARG;

    return phfs_handlers[pdn].write(phfs_handlers[pdn].dn, handle, pos, buff, len, sync);
}


s32 phfs_close(u16 pdn, s32 handle)
{
    if (pdn > PDN_NB)
        return ERR_ARG;

    return phfs_handlers[pdn].close(phfs_handlers[pdn].dn, handle);
}


void phfs_init(void)
{
    int i;

    /* Handlers for flash memories */
    for (i = 0; i < 2; ++i) {
        phfs_handlers[PDN_FLASH0 + i].open = flash_open;
        phfs_handlers[PDN_FLASH0 + i].read = flash_read;
        phfs_handlers[PDN_FLASH0 + i].write = flash_write;
        phfs_handlers[PDN_FLASH0 + i].close = flash_close;
        phfs_handlers[PDN_FLASH0 + i].dn = i;
    }

    return;
}
