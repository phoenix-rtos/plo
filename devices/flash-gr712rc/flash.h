/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR712RC Flash on PROM interface driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _FLASH_H_
#define _FLASH_H_


#include <hal/hal.h>

/* Timeouts in ms */
#define CFI_TIMEOUT_MAX_PROGRAM(typical, maximum) (((1u << typical) * (1u << maximum)) / 1000u)
#define CFI_TIMEOUT_MAX_ERASE(typical, maximum)   ((1u << typical) * (1u << maximum))

#define CFI_SIZE(size) (1u << (u32)size)

#define FLASH_BLOCKSZ 0x20000


typedef struct {
	u8 wordProgram;
	u8 bufWrite;
	u8 blkErase;
	u8 chipErase;
} __attribute__((packed)) flash_cfi_timeout_t;


typedef struct {
	u8 vendorData[0x10];
	u8 qry[3];
	u16 cmdSet1;
	u16 addrExt1;
	u16 cmdSet2;
	u16 addrExt2;
	struct {
		u8 vccMin;
		u8 vccMax;
		u8 vppMin;
		u8 vppMax;
	} __attribute__((packed)) voltages;
	flash_cfi_timeout_t toutTypical;
	flash_cfi_timeout_t toutMax;
	u8 chipSz;
	u16 fdiDesc;
	u16 bufSz;
	u8 regionCnt;
	struct {
		u16 count;
		u16 size;
	} __attribute__((packed)) regions[4];
} __attribute__((packed)) flash_cfi_t;


typedef struct {
	flash_cfi_t cfi;

	addr_t ahbStartAddr;
	addr_t blockBufAddr;
	size_t blockSz;
	u8 blockBufDirty;
	u8 blockBuf[FLASH_BLOCKSZ];
} flash_device_t;


int flash_writeBuffer(flash_device_t *dev, addr_t addr, const u8 *data, u8 len);


int flash_blockErase(addr_t blkAddr, time_t timeout);


void flash_read(flash_device_t *dev, addr_t offs, void *buff, size_t len);


void flash_printInfo(int major, int minor, flash_cfi_t *cfi);


int flash_init(flash_device_t *dev);


#endif
