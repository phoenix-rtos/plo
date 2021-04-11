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
#include "../hal.h"

struct {
	u32 buffCnt;
	u8 buff[0x100];
	flash_context_t flash_ctx;
} phfsflash_common[FLASH_NO];


int phfsflash_init(void)
{
	int i;

	if (FLASH_FLEXSPI1_MOUNTED) {
		phfsflash_common[0].flash_ctx.address = FLASH_FLEXSPI1;
		if (flashdrv_init(&phfsflash_common[0].flash_ctx) < 0)
			return ERR_ARG;

		for (i = 0; i < 0x100; ++i)
			phfsflash_common[0].buff[i] = 0xff;
	}

	if (FLASH_FLEXSPI2_MOUNTED) {
		phfsflash_common[1].flash_ctx.address = FLASH_FLEXSPI2;
		if (flashdrv_init(&phfsflash_common[1].flash_ctx) < 0)
			return  ERR_ARG;

		for (i = 0; i < 0x100; ++i)
			phfsflash_common[1].buff[i] = 0xff;
	}

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

	handle.offs = (void *)phfsflash_common[fn].flash_ctx.address;

	return handle;
}


s32 phfsflash_read(u16 fn, handle_t handle, addr_t *pos, u8 *buff, u32 len)
{
	int res;

	if (fn >= FLASH_NO)
		return ERR_ARG;

	if ((res = flashdrv_readData(&phfsflash_common[fn].flash_ctx, *pos, (char *)buff, len)) < 0)
		return res;

	return res;
}


s32 phfsflash_write(u16 fn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync)
{
	u32 size;
	u16 buffOffs = 0;

	size = len;
	if (!len)
		return ERR_NONE;

	if (fn >= FLASH_NO || *pos % 0x100)
		return ERR_ARG;

	while (size) {
		if (size < 0x100) {
			hal_memcpy(phfsflash_common[fn].buff, buff + buffOffs, size);
			if (flashdrv_bufferedPagesWrite(&phfsflash_common[fn].flash_ctx, *pos + buffOffs, (const char *)phfsflash_common[fn].buff, 0x100) < 0)
				return ERR_ARG;
			size = 0;
			break;
		}

		if (flashdrv_bufferedPagesWrite(&phfsflash_common[fn].flash_ctx, *pos + buffOffs, (const char *)(buff + buffOffs), 0x100) < 0)
			return ERR_ARG;

		buffOffs += 0x100;
		size -= 0x100;
	}

	return len - size;
}


s32 phfsflash_close(u16 fn, handle_t handle)
{
	if (fn >= FLASH_NO)
		return ERR_ARG;

	flashdrv_sync(&phfsflash_common[fn].flash_ctx);

	return ERR_NONE;
}
