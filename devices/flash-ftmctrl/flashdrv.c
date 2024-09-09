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

#include <devices/devs.h>
#include <lib/lib.h>

#include <config.h>

#include "flash.h"
#include "ftmctrl.h"


#define FLASH_NO 1u


static struct {
	flash_device_t dev[FLASH_NO];
} fdrv_common;


static int fdrv_isValidAddress(size_t memsz, addr_t offs, size_t len)
{
	if ((offs < memsz) && ((offs + len) <= memsz)) {
		return 1;
	}

	return 0;
}


static int fdrv_directSectorWrite(flash_device_t *dev, addr_t offs, const u8 *src)
{
	addr_t pos;
	int res;
	u8 shift = ((dev->model->chipWidth == 16) && (ftmctrl_portWidth() == 8)) ? 1 : 0;
	size_t bytecount = CFI_SIZE(dev->cfi.bufSz) >> shift;

	ftmctrl_WrEn();

	res = flash_sectorErase(dev, offs, CFI_TIMEOUT_MAX_ERASE(dev->cfi.toutTypical.blkErase, dev->cfi.toutMax.blkErase));
	if (res < 0) {
		ftmctrl_WrDis();
		return res;
	}

	for (pos = 0; pos < dev->sectorSz; pos += bytecount) {
		res = flash_writeBuffer(dev, offs + pos, src + pos, bytecount, CFI_TIMEOUT_MAX_PROGRAM(dev->cfi.toutTypical.bufWrite, dev->cfi.toutMax.bufWrite));
		if (res < 0) {
			ftmctrl_WrDis();
			return res;
		}
	}

	ftmctrl_WrDis();

	hal_cpuInvCache(hal_cpuDCache, ADDR_FLASH + offs, dev->sectorSz);

	return pos;
}


/* Device driver interface */


static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	flash_device_t *dev;

	(void)timeout;

	if (minor >= FLASH_NO) {
		return -ENXIO;
	}

	dev = &fdrv_common.dev[minor];

	if (fdrv_isValidAddress(CFI_SIZE(dev->cfi.chipSz), offs, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	if (dev->sectorBufAddr == flash_getSectorAddress(dev, offs)) {
		hal_memcpy(buff, dev->sectorBuf + (offs - dev->sectorBufAddr), len);
		return len;
	}

	ftmctrl_WrEn();
	flash_read(dev, offs, buff, len);
	ftmctrl_WrDis();

	return len;
}


static int flashdrv_sync(unsigned int minor)
{
	flash_device_t *dev;

	if (minor >= FLASH_NO) {
		return -ENXIO;
	}

	dev = &fdrv_common.dev[minor];

	if ((dev->sectorBufAddr == (addr_t)-1) || (dev->sectorBufDirty == 0)) {
		return EOK;
	}

	return fdrv_directSectorWrite(dev, dev->sectorBufAddr, dev->sectorBuf);
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	addr_t blkOffs, currAddr;
	const u8 *src = buff;
	size_t doneBytes = 0, chunk;
	flash_device_t *dev;

	if (minor >= FLASH_NO) {
		return -ENXIO;
	}

	dev = &fdrv_common.dev[minor];

	if (fdrv_isValidAddress(CFI_SIZE(dev->cfi.chipSz), offs, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	while (doneBytes < len) {
		currAddr = flash_getSectorAddress(dev, offs);

		blkOffs = offs - currAddr;
		chunk = min(dev->sectorSz - blkOffs, len - doneBytes);

		if (currAddr != dev->sectorBufAddr) {
			if ((blkOffs == 0) && (chunk == dev->sectorSz)) {
				/* Whole block to write */
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
				ftmctrl_WrEn();
				flash_read(dev, dev->sectorBufAddr, dev->sectorBuf, dev->sectorSz);
				ftmctrl_WrDis();
			}
		}

		if (currAddr == dev->sectorBufAddr) {
			/* Sector to write to in cache */
			hal_memcpy(dev->sectorBuf + blkOffs, src, chunk);
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
	ssize_t res = -ENOSYS;
	addr_t offs = addr, end;
	flash_device_t *dev;

	(void)flags;

	if (minor >= FLASH_NO) {
		return -ENXIO;
	}

	dev = &fdrv_common.dev[minor];

	if (fdrv_isValidAddress(CFI_SIZE(dev->cfi.chipSz), addr, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	if (len == (size_t)-1) {
		/* Erase entire memory */
		dev->sectorBufAddr = (addr_t)-1;
		offs = 0;
		end = CFI_SIZE(dev->cfi.chipSz);
		log_info("\nErasing entire memory ...");
	}
	else {
		/* Erase sectors or blocks */
		offs = flash_getSectorAddress(dev, offs);
		end = flash_getSectorAddress(dev, offs + len + dev->sectorSz - 1u);
		log_info("\nErasing blocks from 0x%x to 0x%x ...", offs, end);
	}

	ftmctrl_WrEn();

	if (len == -1) {
		res = flash_chipErase(dev, CFI_TIMEOUT_MAX_ERASE(dev->cfi.toutTypical.chipErase, dev->cfi.toutMax.chipErase));
		len = CFI_SIZE(dev->cfi.chipSz);
	}

	if (res == -ENOSYS) {
		len = 0;
		while (offs < end) {
			if (offs == dev->sectorBufAddr) {
				dev->sectorBufAddr = (addr_t)-1;
			}
			res = flash_sectorErase(dev, offs, CFI_TIMEOUT_MAX_ERASE(dev->cfi.toutTypical.blkErase, dev->cfi.toutMax.blkErase));
			if (res < 0) {
				break;
			}
			offs += dev->sectorSz;
			len += dev->sectorSz;
		}
	}

	ftmctrl_WrDis();

	hal_cpuInvCache(hal_cpuDCache, ADDR_FLASH + addr, len);

	return (res < 0) ? res : len;
}


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz;
	addr_t fStart;
	flash_device_t *dev;

	if (minor >= FLASH_NO) {
		return -ENXIO;
	}

	dev = &fdrv_common.dev[minor];

	fSz = CFI_SIZE(dev->cfi.chipSz);
	fStart = ADDR_FLASH;
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
	flash_device_t *dev;

	if (minor >= FLASH_NO) {
		return -ENXIO;
	}

	dev = &fdrv_common.dev[minor];

	(void)flashdrv_sync(minor);
	ftmctrl_WrEn();
	flash_read(dev, 0, NULL, 0);
	ftmctrl_WrDis();

	return EOK;
}


static int flashdrv_init(unsigned int minor)
{
	int res;
	flash_device_t *dev;

	if (minor >= FLASH_NO) {
		return -ENXIO;
	}

	dev = &fdrv_common.dev[minor];

	dev->sectorBufAddr = (addr_t)-1;
	dev->sectorBufDirty = 0;

	hal_memset(dev->sectorBuf, 0xff, sizeof(dev->sectorBuf));

	ftmctrl_WrEn();
	res = flash_init(dev);
	ftmctrl_WrDis();
	if (res != EOK) {
		log_error("\ndev/flash: Failed to initialize flash%d", minor);
		return res;
	}

	flash_printInfo(dev, DEV_STORAGE, minor);

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_ops_t opsFlashFtmctrl = {
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = flashdrv_erase,
		.sync = flashdrv_sync,
		.map = flashdrv_map,
	};

	static const dev_t devFlashFtmctrl = {
		.name = "flash-ftmctrl",
		.init = flashdrv_init,
		.done = flashdrv_done,
		.ops = &opsFlashFtmctrl,
	};

	hal_memset(&fdrv_common, 0, sizeof(fdrv_common));

	devs_register(DEV_STORAGE, FLASH_NO, &devFlashFtmctrl);

	amd_register();
	intel_register();
}
