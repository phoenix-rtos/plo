/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Zynq-7000 SD card driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/lib.h>
#include <devices/devs.h>

#include "sdcard.h"

#define SDCARD_SLOT 0 /* ID of the SD card slot to use */
#define REAL_ERASE  0 /* If 0, perform "erase" by writing 0xFF bytes to emulate behavior of NOR flash */


static struct
{
	char dataBuffer[SDCARD_MAX_TRANSFER] __attribute__((aligned(SIZE_PAGE)));
	u32 sizeBl;
	u8 initialized;
} sdcard_common = {
	.initialized = 0
};


int sdcarddrv_init(unsigned int minor)
{
	int ret = sdcard_initHost(SDCARD_SLOT, sdcard_common.dataBuffer);
	if (ret < 0) {
		lib_printf(
			"\ndev/sdcard: Error initializing SD host: %d (%d.%d)",
			ret,
			DEV_STORAGE,
			minor);
		return ret;
	}

	ret = sdcard_initCard(SDCARD_SLOT, 0);
	if (ret < 0) {
		lib_printf(
			"\ndev/sdcard: Error initializing SD card: %d (%d.%d)",
			ret,
			DEV_STORAGE,
			minor);
		return ret;
	}

	sdcard_common.sizeBl = sdcard_getSizeBlocks(SDCARD_SLOT);
	sdcard_common.initialized = 1;
	lib_printf(
		"\ndev/sdcard: Configured SD card, size %uMB (%d.%d)",
		sdcard_common.sizeBl / 2048,
		DEV_STORAGE,
		minor);

	return 0;
}


int sdcarddrv_done(unsigned int minor)
{
	sdcard_free(SDCARD_SLOT);
	sdcard_common.initialized = 0;
	return 0;
}


static ssize_t readwrite(unsigned int minor, addr_t offs, void *buff, const size_t len, time_t deadline, int write)
{
	int ret;

	size_t lenRemaining = len;
	u32 offsBlock = (offs / SDCARD_BLOCKLEN);
	u32 offsRem = offs % SDCARD_BLOCKLEN;
	u32 lenBlocks = (len + offsRem + SDCARD_BLOCKLEN - 1) / SDCARD_BLOCKLEN;

	if (!sdcard_common.initialized) {
		return -EINVAL;
	}

	if ((offsBlock > sdcard_common.sizeBl) || (offsBlock + lenBlocks > sdcard_common.sizeBl)) {
		return -EINVAL;
	}

	if ((offs % SDCARD_BLOCKLEN) != 0) {
		u32 remSize = SDCARD_BLOCKLEN - offsRem;
		if (remSize > lenRemaining) {
			remSize = lenRemaining;
		}

		ret = sdcard_transferBlocks(SDCARD_SLOT, sdio_read, offsBlock, 1, deadline);
		if (ret < 0) {
			return ret;
		}

		if (write) {
			hal_memcpy(sdcard_common.dataBuffer + offsRem, buff, remSize);
			ret = sdcard_transferBlocks(SDCARD_SLOT, sdio_write, offsBlock, 1, deadline);
			if (ret < 0) {
				return ret;
			}
		}
		else {
			hal_memcpy(buff, sdcard_common.dataBuffer + offsRem, remSize);
		}

		buff += remSize;
		lenRemaining -= remSize;
		offsBlock += 1;
	}

	while (lenRemaining != 0) {
		size_t toRead = (SDCARD_MAX_TRANSFER < lenRemaining) ? SDCARD_MAX_TRANSFER : lenRemaining;
		u32 toReadBlocks = toRead / SDCARD_BLOCKLEN;
		if ((toRead % SDCARD_BLOCKLEN) != 0) {
			if (write) {
				if (toReadBlocks > 0) {
					toRead = toReadBlocks * SDCARD_BLOCKLEN;
				}
				else {
					toReadBlocks = 1;
					ret = sdcard_transferBlocks(SDCARD_SLOT, sdio_read, offsBlock, 1, deadline);
					if (ret < 0) {
						return ret;
					}
				}
			}
			else {
				toReadBlocks += 1;
			}
		}

		if (write) {
			hal_memcpy(sdcard_common.dataBuffer, buff, toRead);
		}

		ret = sdcard_transferBlocks(SDCARD_SLOT, write ? sdio_write : sdio_read, offsBlock, toReadBlocks, deadline);
		if (ret < 0) {
			return ret;
		}

		if (!write) {
			hal_memcpy(buff, sdcard_common.dataBuffer, toRead);
		}

		lenRemaining -= toRead;
		buff += toRead;
		offsBlock += toReadBlocks;
	}

	return len;
}


ssize_t sdcarddrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	time_t deadline = hal_timerGet() + timeout;
	return readwrite(minor, offs, buff, len, deadline, 0);
}


ssize_t sdcarddrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	/* Timeout value is a bit arbitrary, because the timing can vary greatly from card to card
	 * The part that depends on length assumes 25 MHz 4-bit transfer mode
	 */
	time_t deadline = hal_timerGet() + 3000 + len / 12500;
	return readwrite(minor, offs, (void *)buff, len, deadline, 1);
}


ssize_t sdcarddrv_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	int ret;
	if (!sdcard_common.initialized) {
		return -EINVAL;
	}

	if ((offs % SDCARD_BLOCKLEN != 0) || (len % SDCARD_BLOCKLEN != 0)) {
		return -EINVAL;
	}

	u32 offsBlock = offs / SDCARD_BLOCKLEN;
	u32 lenBlocks = len / SDCARD_BLOCKLEN;
	if ((offsBlock > sdcard_common.sizeBl) || (offsBlock + lenBlocks > sdcard_common.sizeBl)) {
		return -EINVAL;
	}

	if (REAL_ERASE) {
		u32 erasesz = sdcard_getEraseSizeBlocks(SDCARD_SLOT);
		if ((offsBlock % erasesz != 0) || (lenBlocks % erasesz != 0)) {
			return -EINVAL;
		}

		ret = sdcard_eraseBlocks(SDCARD_SLOT, offsBlock, lenBlocks);
	}
	else {
		ret = sdcard_writeFF(SDCARD_SLOT, offsBlock, lenBlocks);
	}

	if (ret < 0) {
		return ret;
	}

	return len;
}


int sdcarddrv_sync(unsigned int minor)
{
	if (minor >= sdcard_common.initialized) {
		return -EINVAL;
	}

	return 0;
}


static int sdcarddrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	return dev_isNotMappable;
}


__attribute__((constructor)) static void sdcarddrv_reg(void)
{
	static const dev_handler_t h = {
		.init = sdcarddrv_init,
		.done = sdcarddrv_done,
		.read = sdcarddrv_read,
		.write = sdcarddrv_write,
		.erase = sdcarddrv_erase,
		.sync = sdcarddrv_sync,
		.map = sdcarddrv_map,
	};

	devs_register(DEV_STORAGE, 1, &h);
}
