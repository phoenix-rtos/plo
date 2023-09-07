/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * RAM driver - based on the flash driver implementation
 *
 * Copyright 2020, 2022 Phoenix Systems
 * Author: Aleksander Kaminski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include <devices/devs.h>

/* There are in fact 2 SRAM banks, but it's mapped as one on Phoenix-RTOS */
#define RAM_NO 1


static const struct {
	addr_t start;
	addr_t end;
} ramParams[RAM_NO] = {
	{ RAM_ADDR, RAM_ADDR + RAM_BANK_SIZE }
};


static int ramdrv_isValidAddress(unsigned int minor, u32 off, size_t size)
{
	size_t rsize = ramParams[minor].end - ramParams[minor].start;

	if (off < rsize && (off + size) <= rsize)
		return 1;

	return 0;
}


static int ramdrv_isValidMinor(unsigned int minor)
{
	return minor < RAM_NO ? 1 : 0;
}


/* Device interface */
static ssize_t ramdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	char *memptr;
	ssize_t ret = -EINVAL;

	(void)timeout;

	if (ramdrv_isValidMinor(minor) != 0 && ramdrv_isValidAddress(minor, offs, len) != 0) {
		memptr = (void *)ramParams[minor].start;

		hal_memcpy(buff, memptr + offs, len);
		ret = (ssize_t)len;
	}

	return ret;
}


static ssize_t ramdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	if (ramdrv_isValidMinor(minor) == 0) {
		return -EINVAL;
	}

	/* Not supported. TODO? */
	return -ENOSYS;
}


static int ramdrv_done(unsigned int minor)
{
	if (ramdrv_isValidMinor(minor) == 0)
		return -EINVAL;

	/* Nothing to do */

	return EOK;
}


static int ramdrv_sync(unsigned int minor)
{
	if (ramdrv_isValidMinor(minor) == 0)
		return -EINVAL;

	/* Nothing to do */

	return EOK;
}


static int ramdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t rSz;
	addr_t rStart;

	if (ramdrv_isValidMinor(minor) == 0)
		return -EINVAL;

	rStart = ramParams[minor].start;
	rSz = ramParams[minor].end - ramParams[minor].start;

	/* Check if region is located on ram */
	if ((addr + sz) >= rSz)
		return -EINVAL;

	/* Check if ram is mappable to map region */
	if (rStart <= memaddr && (rStart + rSz) >= (memaddr + memsz)) {
		*a = rStart;
		return dev_isMappable;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	/* Data can be copied from device to map */
	return dev_isNotMappable;
}


static int ramdrv_init(unsigned int minor)
{
	if (ramdrv_isValidMinor(minor) == 0)
		return -EINVAL;

	return EOK;
}


__attribute__((constructor)) static void ramdrv_reg(void)
{
	static const dev_handler_t h = {
		.init = ramdrv_init,
		.done = ramdrv_done,
		.read = ramdrv_read,
		.write = ramdrv_write,
		.erase = NULL,
		.sync = ramdrv_sync,
		.map = ramdrv_map
	};

	devs_register(DEV_RAM, RAM_NO, &h);
}
