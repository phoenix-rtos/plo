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

#include "errors.h"
#include "hal.h"
#include "plostd.h"
#include "phfs.h"

/* Configuration file for specific platform */
#include "config.h"


phfs_handler_t phfs_handlers[PDN_NB];


handle_t phfs_open(u16 pdn, const char *name, u32 flags)
{
	handle_t handle;

	if (pdn >= PDN_NB) {
		handle.h = ERR_ARG;
		return handle;
	}

	return phfs_handlers[pdn].open(phfs_handlers[pdn].dn, name, flags);
}


s32 phfs_read(u16 pdn, handle_t handle, addr_t *pos, u8 *buff, u32 len)
{
	if (pdn >= PDN_NB)
		return ERR_ARG;

	return phfs_handlers[pdn].read(phfs_handlers[pdn].dn, handle, pos, buff, len);
}


s32 phfs_write(u16 pdn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync)
{
	if (pdn >= PDN_NB)
		return ERR_ARG;

	return phfs_handlers[pdn].write(phfs_handlers[pdn].dn, handle, pos, buff, len, sync);
}


s32 phfs_close(u16 pdn, handle_t handle)
{
	if (pdn >= PDN_NB)
		return ERR_ARG;

	return phfs_handlers[pdn].close(phfs_handlers[pdn].dn, handle);
}


void phfs_init(void)
{
	hal_initphfs(phfs_handlers);

	return;
}
