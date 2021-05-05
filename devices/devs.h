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
	int (*init)(unsigned int minor);
	int (*done)(unsigned int minor);
	int (*sync)(unsigned int minor);

	ssize_t (*read)(unsigned int minor, addr_t offs, u8 *buff, unsigned int len);
	ssize_t (*write)(unsigned int minor, addr_t offs, const u8 *buff, unsigned int len);
} dev_handler_t;


/* This function should be called only before devs_init(),
 * preferably from device drivers constructors. */
extern void devs_register(unsigned int major, unsigned int nb, dev_handler_t *h);


extern void devs_init(void);


extern int devs_check(unsigned int major, unsigned int minor);


extern int devs_sync(unsigned int major, unsigned int minor);


extern ssize_t devs_read(unsigned int major, unsigned int minor, addr_t offs, u8 *buff, unsigned int len);


extern ssize_t devs_write(unsigned int major, unsigned int minor, addr_t offs, const u8 *buff, unsigned int len);


extern void devs_done(void);


#endif
