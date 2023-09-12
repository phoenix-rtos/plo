/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT hyperflash NOR device driver
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HYPERFLASH_H_
#define _HYPERFLASH_H_


struct nor_device;

struct nor_info {
	u32 jedecId;
	const char *name;
	size_t totalSz;
	size_t pageSz;
	size_t sectorSz;
	size_t lutSz;
	const u32 *lut;
	int (*init)(struct nor_device *);
};


enum {
	fspi_hyperReadData = 0,
	fspi_hyperWriteData = 1,
	fspi_hyperReadStatus = 2,
	fspi_hyperWriteEnable = 4,
	fspi_hyperEraseSector = 6,
	fspi_hyperPageProgram = 10,
};


extern ssize_t hyper_readData(flexspi_t *fspi, addr_t addr, void *data, size_t size, time_t timeout);


extern ssize_t hyper_writeData(flexspi_t *fspi, addr_t addr, const void *data, size_t size, time_t timeout);


extern int hyper_writeEnable(flexspi_t *fspi, addr_t addr, time_t timeout);


extern int hyper_readStatus(flexspi_t *fspi, u16 *statusWord, time_t timeout);


extern int hyper_waitBusy(flexspi_t *fspi, time_t timeout);


extern int hyper_eraseSector(flexspi_t *fspi, addr_t addr, time_t timeout);


extern int hyper_pageProgram(flexspi_t *fspi, addr_t dstAddr, const void *src, size_t pageSz, time_t timeout);


extern int hyper_probe(flexspi_t *fspi, const struct nor_info **pInfo, time_t timeout);


#endif /* _HYPERFLASH_H_ */
