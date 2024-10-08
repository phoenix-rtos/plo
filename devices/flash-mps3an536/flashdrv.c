/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * MPS3 AN536 Flash driver
 *
 * Copyright 2020, 2024 Phoenix Systems
 * Author: Aleksander Kaminski, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include <devices/devs.h>


#define FLASH_START_ADDR 0x08000000
#define FLASH_SIZE       0x00800000


/* On real target there's QSPI flash, but QEMU emulates it as a simple ROM */


static const struct {
	u32 start;
	u32 end;
} flashParams = {
	FLASH_START_ADDR, FLASH_START_ADDR + FLASH_SIZE
};


static int flashdrv_isValidAddress(u32 off, size_t size)
{
	size_t fsize = flashParams.end - flashParams.start;

	if ((off < fsize) && ((off + size) <= fsize)) {
		return 1;
	}

	return 0;
}


/* Device interface */
static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	char *memptr;
	ssize_t ret = -EINVAL;

	(void)timeout;

	if ((minor == 0) && (flashdrv_isValidAddress(offs, len) != 0)) {
		memptr = (void *)flashParams.start;

		hal_memcpy(buff, memptr + offs, len);
		ret = (ssize_t)len;
	}

	return ret;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	/* Not supported */
	return -ENOSYS;
}


static int flashdrv_done(unsigned int minor)
{
	if (minor != 0) {
		return -EINVAL;
	}

	/* Nothing to do */

	return EOK;
}


static int flashdrv_sync(unsigned int minor)
{
	if (minor != 0) {
		return -EINVAL;
	}

	/* Nothing to do */

	return EOK;
}


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz;
	addr_t fStart;

	if (minor != 0) {
		return -EINVAL;
	}

	fStart = flashParams.start;
	fSz = flashParams.end - flashParams.start;
	*a = fStart;

	/* Check if region is located on flash */
	if ((addr + sz) >= fSz) {
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

	/* Data can be copied from device to map */
	return dev_isNotMappable;
}


static int flashdrv_init(unsigned int minor)
{
	if (minor != 0) {
		return -EINVAL;
	}

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_ops_t opsFlashMPS3AN536 = {
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = NULL,
		.sync = flashdrv_sync,
		.map = flashdrv_map,
	};

	static const dev_t devFlashMPS3AN536 = {
		.name = "flash-mps3an536",
		.init = flashdrv_init,
		.done = flashdrv_done,
		.ops = &opsFlashMPS3AN536,
	};

	devs_register(DEV_STORAGE, 1, &devFlashMPS3AN536);
}
