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
	__attribute__((aligned(USB_BUFFER_SIZE))) u8 data[SIZE_PHY_BUFF];
	u32 buffCounter;
} phyusb_common;


void *usbclient_allocBuff(u32 size)
{
	void *mem;

	if ((size % USB_BUFFER_SIZE) || (USB_BUFFER_SIZE * phyusb_common.buffCounter + size) > SIZE_PHY_BUFF)
		return NULL;

	mem = (void *)(&phyusb_common.data[USB_BUFFER_SIZE * phyusb_common.buffCounter]);
	phyusb_common.buffCounter += size / USB_BUFFER_SIZE;

	return mem;
}


void usbclient_buffReset(void)
{
	phyusb_common.buffCounter = 0;
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
	phyusb_common.buffCounter = 0;
}
