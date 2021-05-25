/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * USB physical layer
 *
 * Copyright 2020-2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PHY_H_
#define _PHY_H_

#include <lib/types.h>


/* Function returns buffer which is a multiple of USB_BUFFER_SIZE.
 * Otherwise it returns null.                                               */
extern void *usbclient_allocBuff(u32 size);


/* Function cleans the whole memory assigned to endpoints and setup memory. */
extern void usbclient_buffReset(void);


/* Function returns ID of USB controller interrupt.                         */
extern u32 phy_getIrq(void);


/* Function returns physical address of USB controller.                     */
extern void *phy_getBase(void);


/* Function initializes pins and clocks associated with USB controller.     */
extern void phy_init(void);


/* Function resets physical layer of USB controller.                        */
extern void phy_reset(void);


#endif
