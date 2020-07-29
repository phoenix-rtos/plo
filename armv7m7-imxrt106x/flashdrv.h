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

#include "../types.h"
#include "rom_api.h"


#define FLASH_NO                     2
#define FLASH_EXT_DATA_ADDRESS       0x60000000
#define FLASH_INTERNAL_DATA_ADDRESS  0x70000000

#define DEFAULT_SECTOR_SIZE            0x1000

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
    u32 instance;
    u32 flashID;

    int sectorID;
    int counter;

    char buff[DEFAULT_SECTOR_SIZE];
} flash_context_t;


s32 flash_readData(flash_context_t *ctx, u32 offset, char *buff, u32 size);


s32 flash_directBytesWrite(flash_context_t *ctx, u32 offset, const char *buff, u32 size);


s32 flash_bufferedPagesWrite(flash_context_t *ctx, u32 offset, const char *buff, u32 size);


void flash_sync(flash_context_t *ctx);


int flash_chipErase(flash_context_t *ctx);


int flash_sectorErase(flash_context_t *ctx, u32 offset);


int flash_init(flash_context_t *ctx);


void flash_contextDestroy(flash_context_t *ctx);


#endif
