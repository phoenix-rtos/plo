/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB SPIMCTRL Flash driver
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


static const struct {
	vu32 *base;
	addr_t maddr;
} fdrv_info[] = {
	{ SPIMCTRL0_BASE, FLASH0_ADDR },
};


static struct {
	spimctrl_t dev[FLASH_CNT];
} fdrv_common;


/* Auxiliary helpers */


static spimctrl_t *fdrv_minorToDevice(unsigned int minor)
{
	if (minor >= FLASH_CNT) {
		return NULL;
	}

	return &fdrv_common.dev[minor];
}


static inline addr_t fdrv_getSectorAddress(struct nor_device *dev, addr_t addr)
{
	return addr & ~(dev->info->sectorSz - 1u);
}


static int fdrv_isValidAddress(size_t memsz, addr_t offs, size_t len)
{
	if ((offs < memsz) && ((offs + len) <= memsz)) {
		return 1;
	}

	return 0;
}


static ssize_t fdrv_directSectorWrite(spimctrl_t *spimctrl, addr_t offs, const u8 *src)
{
	addr_t pos;
	int res = nor_eraseSector(spimctrl, offs, spimctrl->dev.info->tSE);
	if (res < 0) {
		return res;
	}

	for (pos = 0; pos < spimctrl->dev.info->sectorSz; pos += spimctrl->dev.info->pageSz) {
		res = nor_pageProgram(spimctrl, offs + pos, src + pos, spimctrl->dev.info->pageSz, spimctrl->dev.info->tPP);
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
	spimctrl_t *spimctrl = fdrv_minorToDevice(minor);
	size_t doneBytes = 0;

	if (spimctrl == NULL) {
		return -ENXIO;
	}

	if (fdrv_isValidAddress(spimctrl->dev.info->totalSz, offs, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	if (fdrv_getSectorAddress(&spimctrl->dev, offs) == spimctrl->dev.sectorBufAddr) {
		doneBytes = min(spimctrl->dev.sectorBufAddr + spimctrl->dev.info->sectorSz - offs, len);

		hal_memcpy(buff, spimctrl->dev.sectorBuf + offs - spimctrl->dev.sectorBufAddr, doneBytes);

		if (len == doneBytes) {
			return doneBytes;
		}

		offs += doneBytes;
		buff += doneBytes;
		len -= doneBytes;
	}

	res = nor_readData(spimctrl, offs, buff, len);

	return (res < 0) ? res : (doneBytes + res);
}


static int flashdrv_sync(unsigned int minor)
{
	ssize_t res;
	spimctrl_t *spimctrl = fdrv_minorToDevice(minor);

	if (spimctrl == NULL) {
		return -ENXIO;
	}

	if ((spimctrl->dev.sectorBufAddr == (addr_t)-1) || (spimctrl->dev.sectorBufDirty == 0)) {
		return EOK;
	}

	res = fdrv_directSectorWrite(spimctrl, spimctrl->dev.sectorBufAddr, spimctrl->dev.sectorBuf);
	if (res < 0) {
		return res;
	}
	spimctrl->dev.sectorBufDirty = 0;

	return EOK;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	addr_t sectorOfs, currAddr;
	const u8 *src = buff;
	size_t doneBytes = 0, chunk;
	spimctrl_t *spimctrl = fdrv_minorToDevice(minor);

	if (spimctrl == NULL) {
		return -ENXIO;
	}

	if (fdrv_isValidAddress(spimctrl->dev.info->totalSz, offs, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	while (doneBytes < len) {
		currAddr = fdrv_getSectorAddress(&spimctrl->dev, offs);

		sectorOfs = offs - currAddr;
		chunk = min(spimctrl->dev.info->sectorSz - sectorOfs, len - doneBytes);

		if (currAddr != spimctrl->dev.sectorBufAddr) {
			if ((sectorOfs == 0) && (chunk == spimctrl->dev.info->sectorSz)) {
				/* Whole sector to write */
				res = fdrv_directSectorWrite(spimctrl, offs, src);
				if (res < 0) {
					return res;
				}
			}
			else {
				res = flashdrv_sync(minor);
				if (res < 0) {
					return res;
				}

				spimctrl->dev.sectorBufAddr = currAddr;
				res = nor_readData(spimctrl, spimctrl->dev.sectorBufAddr, spimctrl->dev.sectorBuf, spimctrl->dev.info->sectorSz);
				if (res < 0) {
					spimctrl->dev.sectorBufAddr = (addr_t)-1;
					return res;
				}
			}
		}

		if (currAddr == spimctrl->dev.sectorBufAddr) {
			/* Sector to write to in cache */
			hal_memcpy(spimctrl->dev.sectorBuf + sectorOfs, src, chunk);
			spimctrl->dev.sectorBufDirty = 1;
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
	spimctrl_t *spimctrl = fdrv_minorToDevice(minor);

	(void)flags;

	if (spimctrl == NULL) {
		return -ENXIO;
	}

	if (fdrv_isValidAddress(spimctrl->dev.info->totalSz, addr, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	/* Chip Erase */

	if (len == (size_t)-1) {
		spimctrl->dev.sectorBufAddr = (addr_t)-1;
		len = spimctrl->dev.info->totalSz;
		log_info("\nErasing all data from flash device ...");
		res = nor_eraseChip(spimctrl, spimctrl->dev.info->tCE);

		return (res < 0) ? res : (ssize_t)(len);
	}

	/* Erase sectors or blocks */

	addr = fdrv_getSectorAddress(&spimctrl->dev, addr);
	end = fdrv_getSectorAddress(&spimctrl->dev, addr + len + spimctrl->dev.info->sectorSz - 1u);

	log_info("\nErasing sectors from 0x%x to 0x%x ...", addr, end);

	len = 0;
	while (addr < end) {
		if (addr == spimctrl->dev.sectorBufAddr) {
			spimctrl->dev.sectorBufAddr = (addr_t)-1;
		}
		res = nor_eraseSector(spimctrl, addr, spimctrl->dev.info->tSE);
		if (res < 0) {
			return res;
		}
		addr += spimctrl->dev.info->sectorSz;
		len += spimctrl->dev.info->sectorSz;
	}

	return len;
}


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz;
	addr_t fStart;
	spimctrl_t *spimctrl = fdrv_minorToDevice(minor);

	if (spimctrl == NULL) {
		return -ENXIO;
	}

	fSz = spimctrl->dev.info->totalSz;
	fStart = spimctrl->maddr;
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
	spimctrl_t *spimctrl = fdrv_minorToDevice(minor);

	if (spimctrl == NULL) {
		return -ENXIO;
	}

	spimctrl_reset(spimctrl);

	return EOK;
}


static int flashdrv_init(unsigned int minor)
{
	int res;
	const char *vendor;
	spimctrl_t *spimctrl = fdrv_minorToDevice(minor);

	if (spimctrl == NULL) {
		return -ENXIO;
	}

	spimctrl->base = fdrv_info[minor].base;
	spimctrl->maddr = fdrv_info[minor].maddr;

	spimctrl_init(spimctrl, minor);

	res = nor_deviceInit(spimctrl, &vendor);
	if (res != EOK) {
		log_error("\ndev/flash: Initialization failed");
		return res;
	}

	lib_printf("\ndev/flash/nor: Configured %s %s %dMB nor flash(%d.%d)",
			vendor, spimctrl->dev.info->name, spimctrl->dev.info->totalSz >> 20, DEV_STORAGE, minor);

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_ops_t opsFlashSpimctrl = {
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = flashdrv_erase,
		.sync = flashdrv_sync,
		.map = flashdrv_map,
	};

	static const dev_t devFlashSpimctrl = {
		.name = "flash-spimctrl",
		.init = flashdrv_init,
		.done = flashdrv_done,
		.ops = &opsFlashSpimctrl,
	};

	hal_memset(&fdrv_common, 0, sizeof(fdrv_common));

	devs_register(DEV_STORAGE, FLASH_CNT, &devFlashSpimctrl);
}
