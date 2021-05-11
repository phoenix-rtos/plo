/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * Devices Interface
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _DEVS_H_
#define _DEVS_H_

#include "types.h"
#include "config.h"

#define DEV_UART    0
#define DEV_USB     1
#define DEV_FLASH   2


enum { dev_isMappable = 0, dev_isNotMappable };


typedef struct {
	int (*init)(unsigned int minor);
	int (*done)(unsigned int minor);
	int (*sync)(unsigned int minor);
	int (*map)(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a);

	ssize_t (*read)(unsigned int minor, addr_t offs, u8 *buff, unsigned int len, unsigned int timeout);
	ssize_t (*write)(unsigned int minor, addr_t offs, const u8 *buff, unsigned int len);
} dev_handler_t;


/* This function should be called only before devs_init(),
 * preferably from device drivers constructors */
extern void devs_register(unsigned int major, unsigned int nb, const dev_handler_t *h);


/* Initialize registered devices */
extern void devs_init(void);


/* Check whether device is available */
extern int devs_check(unsigned int major, unsigned int minor);


/* Make synchronization on device. Preferred usage after write function */
extern int devs_sync(unsigned int major, unsigned int minor);


/* Check whether device's region is mappable to map's region. If device is mappable, the device address in memory map is written to a */
extern int devs_map(unsigned int major, unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a);


/* Read data from device */
extern ssize_t devs_read(unsigned int major, unsigned int minor, addr_t offs, u8 *buff, unsigned int len, unsigned int timeout);


/* Write data to device */
extern ssize_t devs_write(unsigned int major, unsigned int minor, addr_t offs, const u8 *buff, unsigned int len);


/* Reset registered devices */
extern void devs_done(void);


#endif
