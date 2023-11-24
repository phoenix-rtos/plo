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

#include <devices/devs.h>
#include <lib/lib.h>

#include "flash.h"
#include "ftmctrl.h"


#define FLASH_NO 1u


static struct {
	flash_device_t dev[FLASH_NO];
} fdrv_common;


static addr_t fdrv_getBlockAddress(flash_device_t *dev, addr_t addr)
{
	return addr & ~(dev->blockSz - 1);
}


static int fdrv_isValidAddress(size_t memsz, addr_t offs, size_t len)
{
	if ((offs < memsz) && ((offs + len) <= memsz)) {
		return 1;
	}

	return 0;
}


static int fdrv_directBlockWrite(flash_device_t *dev, addr_t offs, const u8 *src)
{
	addr_t pos;
	int res;

	ftmctrl_WrEn();

	res = flash_blockErase(offs, CFI_TIMEOUT_MAX_ERASE(dev->cfi.toutTypical.blkErase, dev->cfi.toutMax.blkErase));
	if (res < 0) {
		ftmctrl_WrDis();
		return res;
	}

	for (pos = 0; pos < dev->blockSz; pos += CFI_SIZE(dev->cfi.bufSz)) {
		res = flash_writeBuffer(dev, offs + pos, src + pos, CFI_SIZE(dev->cfi.bufSz));
		if (res < 0) {
			ftmctrl_WrDis();
			return res;
		}
	}

	ftmctrl_WrDis();
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

	if ((dev->blockBufAddr == (addr_t)-1) || (dev->blockBufDirty == 0)) {
		return EOK;
	}

	return fdrv_directBlockWrite(dev, dev->blockBufAddr, dev->blockBuf);
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
		currAddr = fdrv_getBlockAddress(dev, offs);

		blkOffs = offs - currAddr;
		chunk = min(dev->blockSz - blkOffs, len - doneBytes);

		if (currAddr != dev->blockBufAddr) {
			if ((blkOffs == 0) && (chunk == dev->blockSz)) {
				/* Whole block to write */
				res = fdrv_directBlockWrite(dev, offs, src);
				if (res < 0) {
					return res;
				}
			}
			else {
				res = flashdrv_sync(minor);
				if (res < 0) {
					return res;
				}

				dev->blockBufAddr = currAddr;
				ftmctrl_WrEn();
				flash_read(dev, dev->blockBufAddr, dev->blockBuf, dev->blockSz);
				ftmctrl_WrDis();
			}
		}

		if (currAddr == dev->blockBufAddr) {
			/* Sector to write to in cache */
			hal_memcpy(dev->blockBuf + blkOffs, src, chunk);
			dev->blockBufDirty = 1;
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
		dev->blockBufAddr = (addr_t)-1;
		addr = 0;
		end = CFI_SIZE(dev->cfi.chipSz);
		lib_printf("\nErasing entire memory ...");
	}
	else {
		/* Erase sectors or blocks */
		addr = fdrv_getBlockAddress(dev, addr);
		end = fdrv_getBlockAddress(dev, addr + len + dev->blockSz - 1u);
		lib_printf("\nErasing blocks from 0x%x to 0x%x ...", addr, end);
	}

	len = 0;

	ftmctrl_WrEn();

	while (addr < end) {
		if (addr == dev->blockBufAddr) {
			dev->blockBufAddr = (addr_t)-1;
		}
		res = flash_blockErase(addr, CFI_TIMEOUT_MAX_ERASE(dev->cfi.toutTypical.blkErase, dev->cfi.toutMax.blkErase));
		if (res < 0) {
			ftmctrl_WrDis();
			return res;
		}
		addr += dev->blockSz;
		len += dev->blockSz;
	}

	ftmctrl_WrDis();

	return len;
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
	fStart = dev->ahbStartAddr;
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

	hal_memset(dev->blockBuf, 0xff, sizeof(dev->blockBuf));

	ftmctrl_WrEn();
	res = flash_init(dev);
	ftmctrl_WrDis();
	if (res != EOK) {
		log_error("\ndev/flash: Failed to initialize flash%d", minor);
		return res;
	}

	flash_printInfo(DEV_STORAGE, minor, &dev->cfi);

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
