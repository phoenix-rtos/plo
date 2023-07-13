/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT NOR flash device driver
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <lib/lib.h>
#include <devices/devs.h>

#include "lut.h"
#include "fspi.h"
#include "nor/nor.h"


struct {
	struct nor_device dev[FLEXSPI_PORTS];
} fdrv_common;


/* Auxiliary helpers */


static inline int minorToInstance(unsigned int minor)
{
	switch (minor) {
		case 0:
			return flexspi_instance1;

#if defined(__CPU_IMXRT106X) || defined(__CPU_IMXRT117X)
		case 1:
			return flexspi_instance2;
#endif

		default:
			return -1;
	}
}


static inline int minorToPortMask(unsigned int minor)
{
	switch (minor) {
		case 0:
			return flexspi_slBusA1;

#if defined(__CPU_IMXRT106X)
		case 1:
			return flexspi_slBusA1;
#endif

#if defined(__CPU_IMXRT117X)
		case 1:
			return flexspi_slBusA1 | flexspi_slBusA2;
#endif

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
	return addr & ~(dev->nor->sectorSz - 1);
}


/* Device driver interface */


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz;
	addr_t fStart;
	struct nor_device *dev = minorToDevice(minor);

	if (dev == NULL || dev->active == 0) {
		return -ENXIO;
	}

	fSz = dev->fspi.slFlashSz[dev->port];
	fStart = dev->fspi.ahbAddr;
	*a = fStart;

	/* Check if region is located on flash */
	if ((addr + sz) > fSz) {
		return -EINVAL;
	}

	/* Check if flash is mappable to map region */
	if (fStart <= memaddr && (fStart + fSz) >= (memaddr + memsz)) {
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
	addr_t ofs, pos, sectorAddr = get_sectorAddress(dev, dev->sectorPrevAddr);

	if (dev == NULL || dev->active == 0) {
		return -ENXIO;
	}

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
		res = nor_eraseSector(&dev->fspi, dev->port, sectorAddr, dev->timeout);
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

		res = nor_pageProgram(&dev->fspi, dev->port, sectorAddr + ofs, dev->sectorBuf + ofs, dev->nor->pageSz, dev->timeout);
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

		if (dev->nor->totalSz < dstAddr + dev->nor->sectorSz) {
			return -EIO;
		}

		res = nor_readData(&dev->fspi, dev->port, get_sectorAddress(dev, dstAddr), dev->sectorBuf, dev->nor->sectorSz, dev->timeout);
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

	if (dev == NULL || dev->active == 0) {
		return -ENXIO;
	}

	if (data == NULL || dstAddr + size > dev->fspi.slFlashSz[dev->port]) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	doneBytes = 0;
	while (doneBytes < size) {
		res = bufferSync(minor, dstAddr, 0);
		if (res < 0) {
			return res; /* FIXME: should error or doneBytes to be returned ? */
		}

		chunkSz = size - doneBytes;
		ofs = dstAddr & (dev->nor->sectorSz - 1);

		if (chunkSz > dev->nor->sectorSz - ofs) {
			chunkSz = dev->nor->sectorSz - ofs;
		}

		hal_memcpy(dev->sectorBuf + ofs, srcPtr, chunkSz);

		dstAddr += chunkSz;
		srcPtr += chunkSz;
		doneBytes += chunkSz;
	}

	if (doneBytes > 0) {
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

	if (dev == NULL || dev->active == 0) {
		return -ENXIO;
	}

	if (data == NULL || addr + size > dev->fspi.slFlashSz[dev->port]) {
		return -EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	if (timeout == 0) {
		timeout = dev->timeout;
	}

	return nor_readData(&dev->fspi, dev->port, addr, data, size, timeout);
}


static ssize_t flashdrv_erase(unsigned int minor, addr_t addr, size_t len, unsigned int flags)
{
	int res, dieCount;
	u32 capFlags;
	addr_t end, addr_mask;

	struct nor_device *dev = minorToDevice(minor);

	(void)flags;

	if (dev == NULL || dev->active == 0) {
		return -ENXIO;
	}

	if (addr + len > dev->fspi.slFlashSz[dev->port] && len != (size_t)-1) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	/* Invalidate sync */
	dev->sectorPrevAddr = (addr_t)-1;
	dev->sectorSyncAddr = (addr_t)-1;

	capFlags = dev->nor->capFlags;

	/* Chip Erase */
	if (len == (size_t)-1) {
		len = dev->fspi.slFlashSz[dev->port];
		if ((capFlags & NOR_CAPS_DIE4) != 0) {
			dieCount = 4;
		}
		else if ((capFlags & NOR_CAPS_DIE2) != 0) {
			dieCount = 2;
		}
		else {
			dieCount = 1;
		}
		lib_printf("\nErasing all data from flash device ...");

		res = nor_eraseChipDie(&dev->fspi, dev->port, capFlags, dieCount, len / dieCount, dev->timeout);
		if (res < 0) {
			return res;
		}

		return len;
	}

	/* Erase sectors or blocks */

	addr_mask = dev->nor->sectorSz - 1;
	end = (addr + len + addr_mask) & ~addr_mask;
	addr &= ~addr_mask;

	lib_printf("\nErasing sectors from 0x%x to 0x%x ...", addr, end);

	len = 0;
	while (addr < end) {
		res = nor_eraseSector(&dev->fspi, dev->port, addr, dev->timeout);
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

	if (dev == NULL || dev->active == 0) {
		return -ENXIO;
	}

	res = flexspi_deinit(&dev->fspi);
	if (res < EOK) {
		return res;
	}

	return EOK;
}


static int flashdrv_init(unsigned int minor)
{
	int port, res;
	const char *vendor;
	size_t flashSz[FLEXSPI_PORTS];
	struct nor_device *dev = minorToDevice(minor);
	int instance = minorToInstance(minor);
	int portMask = minorToPortMask(minor);

	if (dev == NULL) {
		return -ENXIO;
	}

	res = flexspi_init(&dev->fspi, instance, portMask);
	if (res < EOK) {
		return res;
	}

	lib_printf("\ndev/flash: Initialized flexspi%d", instance);

	for (port = 0; port < FLEXSPI_PORTS; ++port) {
		nor_deviceInit(dev, port, 0, NOR_DEFAULT_TIMEOUT);

		if ((portMask & (1 << port)) == 0) {
			continue;
		}

		res = nor_probe(&dev->fspi, port, &dev->nor, &vendor);
		if (res < EOK) {
			continue;
		}

		dev->active = 1;

		if (dev->nor->init) {
			res = dev->nor->init(dev);
			if (res < 0) {
				dev->active = 0;
				lib_printf("\ndev/flash: Initialization failed");
				return res;
			}
		}

		flexspi_lutUpdateEntries(&dev->fspi, 0, dev->nor->lut, LUT_ENTRIES, LUT_SEQSZ);

		hal_memset(flashSz, 0, sizeof(flashSz));
		flashSz[port] = dev->nor->totalSz;
		flexspi_setFlashSize(&dev->fspi, flashSz, FLEXSPI_PORTS);

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
