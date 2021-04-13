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

#include "../types.h"
#include "config.h"

#define DEV_UART    0
#define DEV_USB     1
#define DEV_FLASH   2


typedef struct {
	int (*deinit)(unsigned int dn);
	int (*sync)(unsigned int dn);

	ssize_t (*read)(unsigned int dn, addr_t offs, u8 *buff, unsigned int len);
	ssize_t (*write)(unsigned int dn, addr_t offs, const u8 *buff, unsigned int len);
} dev_handler_t;


/* This function should be called only before devs_init(),
 * preferably from device drivers constructors. */
extern void devs_regDriver(unsigned int dev, int (*init)(unsigned int dn, dev_handler_t *h));


extern void devs_init(void);


extern int devs_sync(unsigned int dev, unsigned int dn);


extern int devs_getHandler(unsigned int dev, unsigned int dn, dev_handler_t **h);


extern ssize_t devs_read(unsigned int dev, unsigned int dn, addr_t offs, u8 *buff, unsigned int len);


extern ssize_t devs_write(unsigned int dev, unsigned int dn, addr_t offs, const u8 *buff, unsigned int len);


extern int devs_deinit(unsigned int dev, unsigned int dn);


extern void devs_deinitAll(void);

#endif
