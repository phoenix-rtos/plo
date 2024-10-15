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
#include "../spimctrl.h"

#define NOR_ERASED_STATE 0xff
#define NOR_SECTORSZ_MAX 0x1000
#define NOR_PAGESZ_MAX   0x100


struct nor_device {
	const struct nor_info *nor;
	spimctrl_t spimctrl;

	addr_t sectorBufAddr;
	u8 sectorBufDirty;
	u8 sectorBuf[NOR_SECTORSZ_MAX];
};


struct nor_info {
	u32 jedecId;
	const char *name;
	size_t totalSz;
	size_t pageSz;
	size_t sectorSz;
	time_t tPP;
	time_t tSE;
	time_t tCE;
};


/* All addresses are relative to NOR base address */


extern int nor_deviceInit(struct nor_device *dev);


extern int nor_waitBusy(spimctrl_t *spimctrl, time_t timeout);


extern int nor_eraseChip(spimctrl_t *spimctrl, time_t timeout);


extern int nor_eraseSector(spimctrl_t *spimctrl, addr_t addr, time_t timeout);


extern int nor_pageProgram(spimctrl_t *spimctrl, addr_t addr, const void *src, size_t len, time_t timeout);


extern ssize_t nor_readData(spimctrl_t *spimctrl, addr_t addr, void *data, size_t size);


extern int nor_probe(spimctrl_t *spimctrl, const struct nor_info **nor, const char **pVendor);


#endif /* _NOR_H_ */
