/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 Flash driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <lib/lib.h>
#include <devices/devs.h>

#include "spimctrl.h"
#include "nor/nor.h"


/* GR716 has 2 SPI memory controllers
 * On GR716-MINI only SPIMCTRL0 is connected to external flash
 */


#define FLASH_NO 1u

static struct {
	struct nor_device dev[SPIMCTRL_NUM];
} fdrv_common;


/* Auxiliary helpers */


static int fdrv_minorToInstance(unsigned int minor)
{
	switch (minor) {
		case 0: return spimctrl_instance0;
		case 1: return spimctrl_instance1;
		default: return -1;
	}
}


static struct nor_device *fdrv_minorToDevice(unsigned int minor)
{
	if (minor >= FLASH_NO) {
		return NULL;
	}

	return &fdrv_common.dev[minor];
}


static inline int fdrv_getSectorIdFromAddress(struct nor_device *dev, addr_t addr)
{
	return addr / dev->nor->sectorSz;
}


static inline addr_t fdrv_getSectorAddress(struct nor_device *dev, addr_t addr)
{
	return addr & ~(dev->nor->sectorSz - 1u);
}


static int fdrv_isValidAddress(size_t memsz, addr_t offs, size_t len)
{
	if ((offs < memsz) && ((offs + len) <= memsz)) {
		return 1;
	}

	return 0;
}


static ssize_t fdrv_directSectorWrite(struct nor_device *dev, addr_t offs, const u8 *src)
{
	addr_t pos;
	int res = nor_eraseSector(&dev->spimctrl, offs, dev->nor->tSE);
	if (res < 0) {
		return res;
	}

	for (pos = 0; pos < dev->nor->sectorSz; pos += dev->nor->pageSz) {
		res = nor_pageProgram(&dev->spimctrl, offs + pos, src + pos, dev->nor->pageSz, dev->nor->tPP);
		if (res < 0) {
			return res;
		}
	}

	return pos;
}


/* Device driver interface */


static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	int res;
	struct nor_device *dev = fdrv_minorToDevice(minor);
	size_t doneBytes = 0;

	if (dev == NULL) {
		return -ENXIO;
	}

	if (fdrv_isValidAddress(dev->nor->totalSz, offs, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	if (fdrv_getSectorAddress(dev, offs) == dev->sectorBufAddr) {
		doneBytes = min(dev->sectorBufAddr + dev->nor->sectorSz - offs, len);

		hal_memcpy(buff, dev->sectorBuf + offs - dev->sectorBufAddr, doneBytes);

		if (len == doneBytes) {
			return doneBytes;
		}

		offs += doneBytes;
		buff += doneBytes;
		len -= doneBytes;
	}

	res = nor_readData(&dev->spimctrl, offs, buff, len);

	return (res < 0) ? res : (doneBytes + res);
}


static int flashdrv_sync(unsigned int minor)
{
	struct nor_device *dev = fdrv_minorToDevice(minor);

	if (dev == NULL) {
		return -ENXIO;
	}

	if ((dev->sectorBufAddr == (addr_t)-1) || (dev->sectorBufDirty == 0)) {
		return EOK;
	}

	return fdrv_directSectorWrite(dev, dev->sectorBufAddr, dev->sectorBuf);
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	addr_t sectorOfs, currAddr;
	const u8 *src = buff;
	size_t doneBytes = 0, chunk;
	struct nor_device *dev = fdrv_minorToDevice(minor);

	if (dev == NULL) {
		return -ENXIO;
	}

	if (fdrv_isValidAddress(dev->nor->totalSz, offs, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	while (doneBytes < len) {
		currAddr = fdrv_getSectorAddress(dev, offs);

		sectorOfs = offs - currAddr;
		chunk = min(dev->nor->sectorSz - sectorOfs, len - doneBytes);

		if (currAddr != dev->sectorBufAddr) {
			if ((sectorOfs == 0) && (chunk == dev->nor->sectorSz)) {
				/* Whole sector to write */
				res = fdrv_directSectorWrite(dev, offs, src);
				if (res < 0) {
					return res;
				}
			}
			else {
				res = flashdrv_sync(minor);
				if (res < 0) {
					return res;
				}

				dev->sectorBufAddr = currAddr;
				res = nor_readData(&dev->spimctrl, dev->sectorBufAddr, dev->sectorBuf, dev->nor->sectorSz);
				if (res < 0) {
					dev->sectorBufAddr = (addr_t)-1;
					return res;
				}
			}
		}

		if (currAddr == dev->sectorBufAddr) {
			/* Sector to write to in cache */
			hal_memcpy(dev->sectorBuf + sectorOfs, src, chunk);
			dev->sectorBufDirty = 1;
		}

		src += chunk;
		offs += chunk;
		doneBytes += chunk;
	}

	return doneBytes;
}


static ssize_t flashdrv_erase(unsigned int minor, addr_t addr, size_t len, unsigned int flags)
{
	ssize_t res;
	addr_t end;
	struct nor_device *dev = fdrv_minorToDevice(minor);

	(void)flags;

	if (dev == NULL) {
		return -ENXIO;
	}

	if (fdrv_isValidAddress(dev->nor->totalSz, addr, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	/* Chip Erase */

	if (len == (size_t)-1) {
		dev->sectorBufAddr = (addr_t)-1;
		len = dev->nor->totalSz;
		lib_printf("\nErasing all data from flash device ...");
		res = nor_eraseChip(&dev->spimctrl, dev->nor->tCE);

		return (res < 0) ? res : (ssize_t)(len);
	}

	/* Erase sectors or blocks */

	addr = fdrv_getSectorAddress(dev, addr);
	end = fdrv_getSectorAddress(dev, addr + len + dev->nor->sectorSz - 1u);

	lib_printf("\nErasing sectors from 0x%x to 0x%x ...", addr, end);

	len = 0;
	while (addr < end) {
		if (addr == dev->sectorBufAddr) {
			dev->sectorBufAddr = (addr_t)-1;
		}
		res = nor_eraseSector(&dev->spimctrl, addr, dev->nor->tSE);
		if (res < 0) {
			return res;
		}
		addr += dev->nor->sectorSz;
		len += dev->nor->sectorSz;
	}

	return len;
}


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz;
	addr_t fStart;
	struct nor_device *dev = fdrv_minorToDevice(minor);

	if (dev == NULL) {
		return -ENXIO;
	}

	fSz = dev->nor->totalSz;
	fStart = dev->spimctrl.ahbStartAddr;
	*a = fStart;

	/* Check if region is located on flash */
	if ((addr + sz) >= fSz) {
		return -EINVAL;
	}

	/* Check if flash is mappable to map region */
	if ((fStart <= memaddr) && (fStart + fSz) >= (memaddr + memsz)) {
		return dev_isMappable;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* flash is not mappable to any region in I/O mode */
	return dev_isNotMappable;
}


static int flashdrv_done(unsigned int minor)
{
	struct nor_device *dev = fdrv_minorToDevice(minor);

	if (dev == NULL) {
		return -ENXIO;
	}

	spimctrl_reset(&dev->spimctrl);

	return EOK;
}


static int flashdrv_init(unsigned int minor)
{
	int res;
	const char *vendor;
	struct nor_device *dev = fdrv_minorToDevice(minor);
	int instance = fdrv_minorToInstance(minor);

	if (dev == NULL) {
		return -ENXIO;
	}

	res = spimctrl_init(&dev->spimctrl, instance);
	if (res != EOK) {
		log_error("\ndev/flash: Failed to initialize spimctrl%d", instance);
		return res;
	}

	log_info("\ndev/flash: Initialized spimctrl%d", instance);

	res = nor_deviceInit(dev);
	if (res != EOK) {
		log_error("\ndev/flash: Initialization failed");
		return res;
	}

	res = nor_probe(&dev->spimctrl, &dev->nor, &vendor);
	if (res != EOK) {
		log_error("\ndev/flash: Initialization failed");
		return res;
	}

	lib_printf("\ndev/flash/nor: Configured %s %s %dMB nor flash(%d.%d)",
		vendor, dev->nor->name, dev->nor->totalSz >> 20, DEV_STORAGE, minor);

	return EOK;
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
		.map = flashdrv_map
	};

	hal_memset(&fdrv_common, 0, sizeof(fdrv_common));

	devs_register(DEV_STORAGE, FLASH_NO, &h);
}
