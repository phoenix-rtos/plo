/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MX RT Flash driver
 *
 * Copyright 2019, 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _IMXRT_FLASHDRV_H_
#define _IMXRT_FLASHDRV_H_

#include "romapi.h"

#include <hal/hal.h>

typedef struct {
	u32 size;
	u32 page_size;
	u32 sector_size;
} flash_properties_t;


typedef struct {
	flash_properties_t properties;
	serial_norConfigOption_t option;
	flexspi_norConfig_t config;

	u32 address;
	u32 maxSize;
	u32 instance;
	u32 flashID;

	int sectorID;
	int counter;

	char buff[FLASH_DEFAULT_SECTOR_SIZE];
} flash_context_t;

#endif
