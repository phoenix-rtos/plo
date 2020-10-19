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

#include "phfs-flash.h"
#include "flashdrv.h"


flash_context_t flash_ctx[FLASH_NO];


int phfsflash_init(void)
{
	/* Initialize internal flash memory */
	flash_ctx[0].address = FLASH_INTERNAL_DATA_ADDRESS;
	if (flashdrv_init(&flash_ctx[0]) < 0)
		return ERR_ARG;

	/* Initialize external flash memory */
	flash_ctx[1].address = FLASH_EXT_DATA_ADDRESS;
	if (flashdrv_init(&flash_ctx[1]) < 0)
		return  ERR_ARG;

	return 0;
}


handle_t phfsflash_open(u16 fn, const char *name, u32 flags)
{
	handle_t handle;
	handle.h = 0;

	if (fn >= FLASH_NO) {
		handle.h = ERR_ARG;
		return handle;
	}

	handle.offs = (void *)flash_ctx[fn].address;

	return handle;
}


s32 phfsflash_read(u16 fn, handle_t handle, addr_t *pos, u8 *buff, u32 len)
{
	if (fn >= FLASH_NO)
		return ERR_ARG;

	if (flashdrv_readData(&flash_ctx[fn], *pos, (char *)buff, len) < 0)
		return ERR_ARG;

	return ERR_NONE;
}


s32 phfsflash_write(u16 fn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync)
{
	if (fn >= FLASH_NO)
		return ERR_ARG;

	if (flashdrv_bufferedPagesWrite(&flash_ctx[fn], *pos, (const char *)buff, len) < 0)
		return ERR_ARG;

	if (sync)
		flashdrv_sync(&flash_ctx[fn]);

	return ERR_NONE;
}


s32 phfsflash_close(u16 fn, handle_t handle)
{
	if (fn >= FLASH_NO)
		return ERR_ARG;

	flashdrv_contextDestroy(&flash_ctx[fn]);

	return ERR_NONE;
}
