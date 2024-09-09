/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB FTMCTRL Flash driver
 *
 * Copyright 2023, 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _FLASH_H_
#define _FLASH_H_


#include "cfi.h"

#include <hal/hal.h>

#define FLASH_SECTORSZ_MAX 0x20000

typedef struct {
	cfi_info_t cfi;
	const cfi_dev_t *model;

	addr_t sectorBufAddr;
	size_t sectorSz;
	u8 sectorBuf[FLASH_SECTORSZ_MAX];
	u8 sectorBufDirty;
} flash_device_t;


static inline addr_t flash_getSectorAddress(flash_device_t *dev, addr_t addr)
{
	return addr & ~(dev->sectorSz - 1);
}


int flash_writeBuffer(flash_device_t *dev, addr_t addr, const u8 *data, size_t len, time_t timeout);


int flash_sectorErase(flash_device_t *dev, addr_t sectorAddr, time_t timeout);


int flash_chipErase(flash_device_t *dev, time_t timeout);


void flash_read(flash_device_t *dev, addr_t offs, void *buff, size_t len);


void flash_printInfo(flash_device_t *dev, int major, int minor);


int flash_init(flash_device_t *dev);


void flash_register(const cfi_dev_t *dev);


#endif
