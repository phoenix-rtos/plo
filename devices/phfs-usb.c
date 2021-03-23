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


#include "phfs-usb.h"
#include "cdc-client.h"

#include "../phoenixd.h"
#include "../errors.h"


struct {
	phfs_clbk_t cblks;
} phfs_usbCommon;



static int phfs_safeWrite(unsigned int dn, const u8 *buff, u16 len)
{
	return cdc_send(dn, (const char *)buff, len);
}


static int phfs_safeRead(unsigned int dn, u8 *buff, u16 len, u16 timeout)
{
	return cdc_recv(dn, (char *)buff, len);
}


void phfs_usbInit(void)
{
	cdc_init();

	phfs_usbCommon.cblks.read = phfs_safeRead;
	phfs_usbCommon.cblks.write = phfs_safeWrite;
}


void phfs_usbDeinit(void)
{
	phfs_usbCommon.cblks.read = NULL;
	phfs_usbCommon.cblks.write = NULL;

	cdc_destroy();
}


handle_t phfs_usbOpen(u16 dn, const char *name, u32 flags)
{
	return phoenixd_open(dn, name, flags, &phfs_usbCommon.cblks);
}


s32 phfs_usbWrite(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len, u8 sync)
{

    return phoenixd_write(dn, handle, pos, buff, len, sync,  &phfs_usbCommon.cblks);
}


s32 phfs_usbRead(u16 dn, handle_t handle, addr_t *pos, u8 *buff, u32 len)
{
    return phoenixd_read(dn, handle, pos, buff, len, &phfs_usbCommon.cblks);
}


s32 phfs_usbClose(u16 dn, handle_t handle)
{
    return phoenixd_close(dn, handle, &phfs_usbCommon.cblks);
}
