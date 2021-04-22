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

#include "config.h"

typedef struct {
	unsigned int pn; /* phfs device number descriptor */
	unsigned int fd; /* file descriptor               */
} handler_t;


/* Protocols callbacks structure */
typedef struct {
	unsigned int dn;
	ssize_t (*read)(unsigned int dn, addr_t offs, u8 *buff, unsigned int len);
	ssize_t (*write)(unsigned int dn, addr_t offs, const u8 *buff, unsigned int len);
} phfs_clbk_t;


/* Initialization functions */

/* Register alias to device based on minor/major number and assign protocol to alias */
extern int phfs_regDev(const char *alias, unsigned int major, unsigned int minor, const char *prot);


/* Register alias to file located in non-volatile memory */
extern int phfs_regFile(const char *alias, addr_t addr, size_t size);


extern int phfs_getFileAddr(handler_t h, addr_t *addr);


extern int phfs_getFileSize(handler_t h, size_t *size);


/* Show devices registered in phfs */
extern void phfs_showDevs(void);



/* Operations on files */

/* Open files based on alias to device and file name.
 * If file abstraction does not exist on device, the NULL value should be passed to file argument. */
extern int phfs_open(const char *alias, const char *file, unsigned int flags, handler_t *handler);


extern ssize_t phfs_read(handler_t handler, addr_t offs, u8 *buff, unsigned int len);


extern ssize_t phfs_write(handler_t handler, addr_t offs, const u8 *buff, unsigned int len);


extern int phfs_close(handler_t handler);


#endif
