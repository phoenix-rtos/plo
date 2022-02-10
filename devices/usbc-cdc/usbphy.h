/*
 * Phoenix-RTOS
 *
 * Operating system loader
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

#include <hal/hal.h>

/* Temporary solution which works fine only for CDC. If the new device's class is added, SIZE_PHY_BUFF should be changed.
 * Memory size for endpoints and setup data for CDC Device:
 * - 2x Control endpoints + 2x setup data + IRQ endpoint + Bulk endpoint = 8 */
/* Size of memory pool aligned to USB_BUFFER_SIZE, used by USB descriptors */
#define USB_POOL_SIZE (8 * USB_BUFFER_SIZE)


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
