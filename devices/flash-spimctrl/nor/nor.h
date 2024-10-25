/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 flash driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _NOR_H_
#define _NOR_H_

#include <hal/hal.h>


#define NOR_ERASED_STATE 0xffu
#define NOR_SECTORSZ_MAX 0x10000u
#define NOR_PAGESZ_MAX   0x100u
#define NOR_REGIONS_MAX  4


struct nor_device {
	const struct nor_info *info;
	const struct nor_cmds *cmds;

	addr_t sectorBufAddr;
	u8 ear;
	u8 sectorBufDirty;
	u8 sectorBuf[NOR_SECTORSZ_MAX];
};


struct nor_cmds {
	u8 rdsr;  /* Read status register */
	u8 wren;  /* Write enable */
	u8 wrdi;  /* Write disable */
	u8 rdear; /* Read bank/extended address register */
	u8 wrear; /* Write bank/extended address register */
	u8 ce;    /* Chip erase */
	u8 se;    /* Sector erase */
	u8 pp;    /* Page program */
	u8 read;
};


struct nor_info {
	u32 jedecId;
	const char *name;
	size_t totalSz;
	size_t pageSz;
	size_t sectorSz; /* Max sector size */
	/* Times in ms */
	time_t tPP;
	time_t tSE;
	time_t tCE;

	struct {
		size_t sectorCnt;
		size_t sectorSz;
	} regions[NOR_REGIONS_MAX];
};


struct _spimctrl_t;


/* All addresses are relative to NOR base address */


int nor_eraseChip(struct _spimctrl_t *spimctrl, time_t timeout);


int nor_eraseSector(struct _spimctrl_t *spimctrl, addr_t addr, time_t timeout);


int nor_pageProgram(struct _spimctrl_t *spimctrl, addr_t addr, const void *src, size_t len, time_t timeout);


ssize_t nor_readData(struct _spimctrl_t *spimctrl, addr_t addr, void *data, size_t size);


int nor_deviceInit(struct _spimctrl_t *spimctrl, const char **vendor);


#endif /* _NOR_H_ */
