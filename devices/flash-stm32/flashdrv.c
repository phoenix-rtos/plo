/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32 Flash driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include <devices/devs.h>

#define FLASH_NO 2


static const struct {
	u32 start;
	u32 end;
} flashParams[FLASH_NO] = {
	{ FLASH_PROGRAM_1_ADDR, FLASH_PROGRAM_1_ADDR + FLASH_PROGRAM_BANK_SIZE },
	{ FLASH_PROGRAM_2_ADDR, FLASH_PROGRAM_2_ADDR + FLASH_PROGRAM_BANK_SIZE }
};


static int flashdrv_isValidAddress(unsigned int minor, u32 off, size_t size)
{
	size_t fsize = flashParams[minor].end - flashParams[minor].start;

	if (off < fsize && (off + size) <= fsize)
		return 1;

	return 0;
}


static int flashdrv_isValidMinor(unsigned int minor)
{
	return minor < FLASH_NO ? 1 : 0;
}


/* Device interface */
static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	char *memptr;
	ssize_t ret = -EINVAL;

	(void)timeout;

	if (flashdrv_isValidMinor(minor) != 0 && flashdrv_isValidAddress(minor, offs, len) != 0) {
		memptr = (void *)flashParams[minor].start;

		hal_memcpy(buff, memptr + offs, len);
		ret = (ssize_t)len;
	}

	return ret;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	/* Not supported. TODO? */
	return -ENOSYS;
}


static int flashdrv_done(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0)
		return -EINVAL;

	/* Nothing to do */

	return EOK;
}


static int flashdrv_sync(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0)
		return -EINVAL;

	/* Nothing to do */

	return EOK;
}


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz;
	addr_t fStart;

	if (flashdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	fStart = flashParams[minor].start;
	fSz = flashParams[minor].end - flashParams[minor].start;
	*a = fStart;

	/* Check if region is located on flash */
	if ((addr + sz) >= fSz) {
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

	/* Data can be copied from device to map */
	return dev_isNotMappable;
}


static int flashdrv_init(unsigned int minor)
{
	if (flashdrv_isValidMinor(minor) == 0)
		return -EINVAL;

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_handler_t h = {
		.init = flashdrv_init,
		.done = flashdrv_done,
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = NULL, /* TODO */
		.sync = flashdrv_sync,
		.map = flashdrv_map
	};

	devs_register(DEV_STORAGE, FLASH_NO, &h);
}
