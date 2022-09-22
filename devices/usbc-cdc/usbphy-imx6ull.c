/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * USB physical layer controller
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "client.h"
#include "usbphy.h"
#include <lib/lib.h>


struct {
	u8 pool[USB_POOL_SIZE];
	size_t usedPools;
} phyusb_common __attribute__((section(".uncached_ddr"), aligned(USB_BUFFER_SIZE)));


void *usbclient_allocBuff(u32 size)
{
	size_t org = USB_BUFFER_SIZE * phyusb_common.usedPools;

	if ((size % USB_BUFFER_SIZE) != 0 || org + size > USB_POOL_SIZE)
		return NULL;

	phyusb_common.usedPools += size / USB_BUFFER_SIZE;

	return &phyusb_common.pool[org];
}


void usbclient_buffReset(void)
{
	phyusb_common.usedPools = 0;
}

void *phy_getBase(void)
{
	return (void *)0x02184000;
}


u32 phy_getIrq(void)
{
	return 75;
}


void phy_reset(void)
{
	/* TODO: reset phy controller */
}


/* TODO: add phy controller
 * Currently PHY controller is initialized by bootROM. */
void phy_init(void)
{
	phyusb_common.usedPools = 0;
}
