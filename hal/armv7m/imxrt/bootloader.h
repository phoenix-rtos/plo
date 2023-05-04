/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ROM-API bootloader interface i.MX RT series
 *
 * Copyright 2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_


/* Serial downloader media */
#define BOOT_DOWNLOADER_USB  (0x11 << 16)
#define BOOT_DOWNLOADER_UART (0x12 << 16)


/* Detect and initialize bootloader */
int bootloader_init(void);


/* Return bootloader version */
u32 bootloader_getVersion(void);


/* Get pointer to vendor copyright string */
const char *bootloader_getVendorString(void);


/* Enter ROM bootloader firmware */
void bootloader_run(u32 bootcfg) __attribute__((noreturn));


#endif /* _BOOTLOADER_H_ */
