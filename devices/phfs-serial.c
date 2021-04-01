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

#include "../errors.h"

#include "../phoenixd.h"
#include "../serial.h"
#include "../hal.h"


struct {
	phfs_clbk_t cblks;
} phfs_serialCommon;



int phfs_serialInit(void)
{
	serial_init();

	phfs_serialCommon.cblks.read = serial_read;
	phfs_serialCommon.cblks.write = serial_safewrite;

	return ERR_NONE;
}


void phfs_serialDeinit(void)
{
	phfs_serialCommon.cblks.read = NULL;
	phfs_serialCommon.cblks.write = NULL;

	serial_done();
}


handle_t phfs_serialOpen(u16 dn, const char *name, u32 flags)
{
	return phoenixd_open(dn, name, flags, &phfs_serialCommon.cblks);
}


s32 phfs_serialWrite(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync)
{
	return phoenixd_write(dn, handle, pos, buff, len, sync,  &phfs_serialCommon.cblks);
}


s32 phfs_serialRead(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len)
{
	return phoenixd_read(dn, handle, pos, buff, len, &phfs_serialCommon.cblks);
}


s32 phfs_serialClose(u16 dn, handle_t handle)
{
	return phoenixd_close(dn, handle, &phfs_serialCommon.cblks);
}

