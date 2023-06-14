/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ull NOR flash device driver
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <lib/lib.h>
#include <devices/devs.h>

#include "qspi.h"
#include "nor/nor.h"


#define FLASH_NO 1u


struct {
	struct nor_device dev[QSPI_PORTS];
} fdrv_common;


/* Auxiliary helpers */


static inline int minorToPortMask(unsigned int minor)
{
	switch (minor) {
		case 0:
			return qspi_slBusA1;

		default:
			return 0;
	}
}


static struct nor_device *minorToDevice(unsigned int minor)
{
	if (minor >= FLASH_NO) {
		return NULL;
	}

	return &fdrv_common.dev[minor];
}


static inline int get_sectorIdFromAddress(struct nor_device *dev, addr_t addr)
{
	return addr / dev->nor->sectorSz;
}


static inline addr_t get_sectorAddress(struct nor_device *dev, addr_t addr)
{
	return addr & ~(dev->nor->sectorSz - 1u);
}


/* Device driver interface */


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz;
	addr_t fStart;
	struct nor_device *dev = minorToDevice(minor);

	if ((dev == NULL) || (dev->active == 0)) {
		return -ENXIO;
	}

	fSz = dev->qspi.slFlashSz[dev->port];
	fStart = dev->qspi.ahbAddr;
	*a = fStart;

	/* Check if region is located on flash */
	if ((addr + sz) > fSz) {
		return -EINVAL;
	}

	/* Check if flash is mappable to map region */
	if ((fStart <= memaddr) && ((fStart + fSz) >= (memaddr + memsz))) {
		return dev_isMappable;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* flash is not mappable to any region in I/O mode */
	return dev_isNotMappable;
}


static int flashdrv_sync(unsigned int minor)
{
	ssize_t res;
	struct nor_device *dev = minorToDevice(minor);
	addr_t ofs, pos, sectorAddr;

	if (dev == NULL || dev->active == 0) {
		return -ENXIO;
	}
	sectorAddr = get_sectorAddress(dev, dev->sectorPrevAddr);

	/* Initial sector value check, nothing to do */
	if (dev->sectorPrevAddr == (addr_t)-1) {
		return EOK;
	}

	if (dev->sectorPrevAddr == dev->sectorSyncAddr) {
		return EOK;
	}


	/* all 'ones' in buffer means sector is erased ... */
	for (pos = 0; pos < dev->nor->sectorSz; ++pos) {
		if (*(dev->sectorBuf + pos) != NOR_ERASED_STATE) {
			break;
		}
	}

	/* ... then erase may be skipped */
	if (pos != dev->nor->sectorSz) {
		res = nor_eraseSector(&dev->qspi, dev->port, sectorAddr, dev->timeout);
		if (res < 0) {
			return res;
		}
	}

	for (ofs = 0; ofs < dev->nor->sectorSz; ofs += dev->nor->pageSz) {
		/* Just erased NOR page contains all 'ones' */
		for (pos = 0; pos < dev->nor->pageSz; ++pos) {
			if (*(dev->sectorBuf + ofs + pos) != NOR_ERASED_STATE) {
				break;
			}
		}

		/* then if buffer is the same skip single page program */
		if (pos == dev->nor->pageSz) {
			continue;
		}

		res = nor_pageProgram(&dev->qspi, dev->port, sectorAddr + ofs, dev->sectorBuf + ofs, dev->nor->pageSz, dev->timeout);
		if (res < 0) {
			return res;
		}
	}


	hal_cpuInvCache(hal_cpuDCache, sectorAddr, dev->nor->sectorSz);

	dev->sectorSyncAddr = dev->sectorPrevAddr;

	return dev->nor->sectorSz;
}


static ssize_t bufferSync(unsigned int minor, addr_t dstAddr, int isLast)
{
	ssize_t res = 0;
	struct nor_device *dev = minorToDevice(minor);
	const int sectorLast = get_sectorIdFromAddress(dev, dev->sectorPrevAddr);
	const int sectorCurr = get_sectorIdFromAddress(dev, dstAddr);

	if (sectorCurr != sectorLast) {
		res = flashdrv_sync(minor);
		if (res < 0) {
			return res;
		}

		if (dev->nor->totalSz <= dstAddr) {
			dev->sectorPrevAddr = (addr_t)-1;
			dev->sectorSyncAddr = (addr_t)-1;
		}

		if (isLast != 0) {
			return res;
		}

		if (dev->nor->totalSz < (dstAddr + dev->nor->sectorSz)) {
			return -EIO;
		}

		res = nor_readData(&dev->qspi, dev->port, get_sectorAddress(dev, dstAddr), dev->sectorBuf, dev->nor->sectorSz, dev->timeout);
		if (res < 0) {
			return res;
		}

		dev->sectorPrevAddr = dstAddr;
	}

	return res;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t dstAddr, const void *data, size_t size)
{
	int res;
	addr_t ofs;
	const u8 *srcPtr = data;
	size_t chunkSz, doneBytes;
	struct nor_device *dev = minorToDevice(minor);

	if ((dev == NULL) || (dev->active == 0)) {
		return -ENXIO;
	}

	if ((data == NULL) || ((dstAddr + size) > dev->qspi.slFlashSz[dev->port])) {
		return -EINVAL;
	}

	if (size == 0u) {
		return 0;
	}

	doneBytes = 0;
	while (doneBytes < size) {
		res = bufferSync(minor, dstAddr, 0);
		if (res < 0) {
			return res; /* FIXME: should error or doneBytes to be returned ? */
		}

		ofs = dstAddr & (dev->nor->sectorSz - 1u);
		chunkSz = min(size - doneBytes, dev->nor->sectorSz - ofs);

		hal_memcpy(dev->sectorBuf + ofs, srcPtr, chunkSz);

		dstAddr += chunkSz;
		srcPtr += chunkSz;
		doneBytes += chunkSz;
	}

	if (doneBytes > 0u) {
		res = bufferSync(minor, dstAddr, 1);
		if (res < 0) {
			return res; /* FIXME: should error or doneBytes to be returned ? */
		}
	}

	return doneBytes;
}


static ssize_t flashdrv_read(unsigned int minor, addr_t addr, void *data, size_t size, time_t timeout)
{
	struct nor_device *dev = minorToDevice(minor);

	if ((dev == NULL) || (dev->active == 0)) {
		return -ENXIO;
	}

	if ((data == NULL) || ((addr + size) > dev->qspi.slFlashSz[dev->port])) {
		return -EINVAL;
	}

	if (size == 0u) {
		return 0;
	}

	if (timeout == 0) {
		timeout = dev->timeout;
	}

	return nor_readData(&dev->qspi, dev->port, addr, data, size, timeout);
}


static ssize_t flashdrv_erase(unsigned int minor, addr_t addr, size_t len, unsigned int flags)
{
	int res;
	addr_t end, addr_mask;

	struct nor_device *dev = minorToDevice(minor);

	(void)flags;

	if ((dev == NULL) || (dev->active == 0)) {
		return -ENXIO;
	}

	if (((addr + len) > dev->qspi.slFlashSz[dev->port]) && (len != (size_t)-1)) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	/* Invalidate sync */
	dev->sectorPrevAddr = (addr_t)-1;
	dev->sectorSyncAddr = (addr_t)-1;

	/* Chip Erase */
	if (len == (size_t)-1) {
		len = dev->qspi.slFlashSz[dev->port];
		log_info("\nErasing all data from flash device ...");
		res = nor_eraseChip(&dev->qspi, dev->port, dev->timeout);
		if (res < 0) {
			return res;
		}

		return len;
	}

	/* Erase sectors or blocks */

	addr_mask = dev->nor->sectorSz - 1u;
	end = (addr + len + addr_mask) & ~addr_mask;
	addr &= ~addr_mask;

	log_info("\nErasing sectors from 0x%x to 0x%x ...", addr, end);

	len = 0;
	while (addr < end) {
		res = nor_eraseSector(&dev->qspi, dev->port, addr, dev->timeout);
		if (res < 0) {
			return res;
		}
		addr += dev->nor->sectorSz;
		len += dev->nor->sectorSz;
	}


	return len;
}


static int flashdrv_done(unsigned int minor)
{
	int res;
	struct nor_device *dev = minorToDevice(minor);

	res = flashdrv_sync(minor);
	if (res < EOK) {
		return res;
	}

	if ((dev == NULL) || (dev->active == 0)) {
		return -ENXIO;
	}

	res = qspi_deinit(&dev->qspi);
	if (res < EOK) {
		return res;
	}

	return EOK;
}


static int flashdrv_init(unsigned int minor)
{
	int port, res;
	const char *vendor;
	size_t flashSz[QSPI_PORTS];
	struct nor_device *dev = minorToDevice(minor);
	int portMask = minorToPortMask(minor);

	if (dev == NULL) {
		return -ENXIO;
	}

	res = qspi_init(&dev->qspi, portMask);
	if (res < EOK) {
		return res;
	}

	for (port = 0; port < QSPI_PORTS; ++port) {
		nor_deviceInit(dev, port, 0, NOR_DEFAULT_TIMEOUT);

		if ((portMask & (1u << port)) == 0) {
			continue;
		}

		res = nor_probe(&dev->qspi, port, &dev->nor, &vendor);
		if (res < EOK) {
			continue;
		}

		dev->active = 1;

		if (dev->nor->init != NULL) {
			res = dev->nor->init(dev);
			if (res < 0) {
				dev->active = 0;
				lib_printf("\ndev/flash: Initialization failed");
				return res;
			}
		}

		qspi_lutUpdate(&dev->qspi, 0, dev->nor->lut, dev->nor->lutSz);

		hal_memset(flashSz, 0, sizeof(flashSz));
		flashSz[port] = dev->nor->totalSz;
		qspi_setFlashSize(&dev->qspi, flashSz);

		lib_printf("\ndev/flash/nor: Configured %s %s %dMB nor flash(%d.%d)",
			vendor, dev->nor->name, dev->nor->totalSz >> 20, DEV_STORAGE, minor);

		return EOK;
	}

	return -ENXIO;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_handler_t h = {
		.init = flashdrv_init,
		.done = flashdrv_done,
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = flashdrv_erase,
		.sync = flashdrv_sync,
		.map = flashdrv_map,
	};

	hal_memset(&fdrv_common, 0, sizeof(fdrv_common));

	devs_register(DEV_STORAGE, FLASH_NO, &h);
}
