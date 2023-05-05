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

#ifndef _BOOTROM_H_
#define _BOOTROM_H_


/* clang-format off */

/* Serial downloader media */
#define BOOT_DOWNLOADER_AUTO (0x10u << 16)
#define BOOT_DOWNLOADER_USB  (0x11u << 16)
#define BOOT_DOWNLOADER_UART (0x12u << 16)

#define BOOT_IMAGE_SELECT(n) ((n) & 3u)

/* clang-format on */


/* Detect and initialize ROM bootloader */
int bootrom_init(void);


/* Return ROM bootloader version */
u32 bootrom_getVersion(void);


/* Get pointer to vendor copyright string */
const char *bootrom_getVendorString(void);


/* Enter ROM bootloader firmware */
void bootrom_run(u32 bootcfg) __attribute__((noreturn));


#endif /* _BOOTROM_H_ */
