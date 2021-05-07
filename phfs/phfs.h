/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * PHoenix FileSystem
 *
 * Copyright 2012, 2021 Phoenix Systems
 * Copyright 2005 Pawel Pisarczyk
 *
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PHFS_H_
#define _PHFS_H_

#include "devs.h"
#include "config.h"

typedef struct {
	unsigned int pd; /* phfs device descriptor */
	unsigned int id; /* file id               */
} handler_t;


typedef struct {
	u32 size;
} phfs_stat_t;


/* Initialization functions */

/* Register alias to device based on minor/major number and assign protocol to alias */
extern int phfs_regDev(const char *alias, unsigned int major, unsigned int minor, const char *prot);


/* Register alias to file located in non-volatile memory */
extern int phfs_regFile(const char *alias, addr_t addr, size_t size);


/* Get file's address based on the given handler */
extern int phfs_getFileAddr(handler_t h, addr_t *addr);


/* Get file's size based on the given handler */
extern int phfs_getFileSize(handler_t h, size_t *size);


/* Show devices registered in phfs */
extern void phfs_showDevs(void);



/* Operations on files */

/* Open files based on alias to device and file name.
 * If file abstraction does not exist on device, the NULL value should be passed to file argument. */
extern int phfs_open(const char *alias, const char *file, unsigned int flags, handler_t *handler);


/* Read data from registered device */
extern ssize_t phfs_read(handler_t handler, addr_t offs, u8 *buff, unsigned int len);


/* Write data to registered device */
extern ssize_t phfs_write(handler_t handler, addr_t offs, const u8 *buff, unsigned int len);


/* Close connection with the device */
extern int phfs_close(handler_t handler);


/* Check whether device's region is mappable to map's region. If device is mappable, the device address in memory map is written to a */
extern int phfs_map(handler_t handler, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a);


extern int phfs_stat(handler_t handler, phfs_stat_t *stat);


#endif
