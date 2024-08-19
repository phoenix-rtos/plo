/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR712RC MRAM on PROM interface driver
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <devices/devs.h>
#include <lib/lib.h>

#include "ftmctrl.h"


#define MRAM_NO 1u

#define MRAM_BANK1 0x00000000
#define MRAM_SIZE  0x00100000


static int fdrv_isValidAddress(addr_t offs, size_t len)
{
	if ((offs < MRAM_SIZE) && ((offs + len) <= MRAM_SIZE)) {
		return 1;
	}

	return 0;
}


/* Device driver interface */


static ssize_t mramdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	(void)timeout;

	if (minor >= MRAM_NO) {
		return -ENXIO;
	}


	if (fdrv_isValidAddress(offs, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	for (size_t l = 0; l < len; l++) {
		((u8 *)buff)[l] = *((u8 *)(MRAM_BANK1 + offs + l));
	}

	return len;
}


static int mramdrv_sync(unsigned int minor)
{
	if (minor >= MRAM_NO) {
		return -ENXIO;
	}

	return EOK;
}


static ssize_t mramdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	size_t l;

	if (minor >= MRAM_NO) {
		return -ENXIO;
	}

	if (fdrv_isValidAddress(offs, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	ftmctrl_WrEn();
	hal_memset((void *)(MRAM_BANK1 + offs), 0x0, len);
	for (l = 0; l < len; l++) {
		*((u8 *)(MRAM_BANK1 + offs + l)) = ((u8 *)buff)[l];
	}
	ftmctrl_WrDis();

	return len;
}


static ssize_t mramdrv_erase(unsigned int minor, addr_t addr, size_t len, unsigned int flags)
{
	(void)flags;

	if (minor >= MRAM_NO) {
		return -ENXIO;
	}

	if (fdrv_isValidAddress(addr, len) == 0) {
		return -EINVAL;
	}

	if (len == 0u) {
		return 0;
	}

	ftmctrl_WrEn();
	hal_memset((void *)(MRAM_BANK1 + addr), 0x0, len);
	ftmctrl_WrDis();

	return len;
}


static int mramdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor >= MRAM_NO) {
		return -ENXIO;
	}

	/* Check if region is located on flash */
	if ((addr + sz) >= MRAM_SIZE) {
		return -EINVAL;
	}

	/* Check if flash is mappable to map region */
	if ((MRAM_BANK1 <= memaddr) && (MRAM_BANK1 + MRAM_SIZE) >= (memaddr + memsz)) {
		return dev_isMappable;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* flash is not mappable to any region in I/O mode */
	return dev_isNotMappable;
}


static int mramdrv_done(unsigned int minor)
{
	if (minor >= MRAM_NO) {
		return -ENXIO;
	}

	return EOK;
}


static int mramdrv_init(unsigned int minor)
{
	if (minor >= MRAM_NO) {
		return -ENXIO;
	}

	ftmctrl_init();

	return EOK;
}


__attribute__((constructor)) static void mramdrv_reg(void)
{
	static const dev_handler_t h = {
		.init = mramdrv_init,
		.done = mramdrv_done,
		.read = mramdrv_read,
		.write = mramdrv_write,
		.erase = mramdrv_erase,
		.sync = mramdrv_sync,
		.map = mramdrv_map
	};

	devs_register(DEV_STORAGE, MRAM_NO, &h);
}
