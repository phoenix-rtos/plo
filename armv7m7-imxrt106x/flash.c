/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MX RT Flash manager
 *
 * Copyright 2019, 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../errors.h"

#include "flash.h"
#include "flashdrv.h"


flash_context_t flash_ctx[FLASH_NO];


/* TODO: read partitions offsets from partition table */
s32 flash_open(u16 fn, char *name, u32 flags)
{
    if (fn >= FLASH_NO)
        return ERR_ARG;

    /* Initialize internal flash memory */
    if (fn == 0 && flash_ctx[fn].address != FLASH_INTERNAL_DATA_ADDRESS) {
        flash_ctx[0].address = FLASH_INTERNAL_DATA_ADDRESS;
        if (flash_init(&flash_ctx[0]) < 0)
            return ERR_ARG;
    }
    /* Initialize external flash memory */
    else if (fn == 1 && flash_ctx[fn].address != FLASH_EXT_DATA_ADDRESS) {
        flash_ctx[1].address = FLASH_EXT_DATA_ADDRESS;
        if (flash_init(&flash_ctx[1]) < 0)
            return ERR_ARG;
    }

    return ERR_NONE;
}


s32 flash_read(u16 fn, s32 handle, u32 *pos, u8 *buff, u32 len)
{
    if (fn >= FLASH_NO)
        return ERR_ARG;

    if (flash_readData(&flash_ctx[fn], *pos, (char *)buff, len) < 0)
        return ERR_ARG;

    return ERR_NONE;
}


s32 flash_write(u16 fn, s32 handle, u32 *pos, u8 *buff, u32 len, u8 sync)
{
    if (fn >= FLASH_NO)
        return ERR_ARG;

    if (flash_bufferedPagesWrite(&flash_ctx[fn], *pos, (const char *)buff, len) < 0)
        return ERR_ARG;

    if (sync)
        flash_sync(&flash_ctx[fn]);

    return ERR_NONE;
}


s32 flash_close(u16 fn, s16 handle)
{
    if (fn >= FLASH_NO)
        return ERR_ARG;

    flash_contextDestroy(&flash_ctx[fn]);

    return ERR_NONE;
}
