/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL NAND raw interface
 *
 * Copyright 2022, 2023 Phoenix Systems
 * Author: Lukasz Kosinski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>

#include "drv.h"
#include "data.h"

/* NOTICE: raw device relies on data device for init. */


/* Linker symbols */
extern u8 nand_raw_page[]; /* NAND page cache */


static ssize_t raw_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	nand_t *nand = nand_get(minor);
	size_t size, rawEraseBlockSz, rawsz, rawPagesz, ret, page, poffs;
	int err;

	if (nand == NULL) {
		return -ENODEV;
	}

	rawEraseBlockSz = (nand->cfg->erasesz / nand->cfg->writesz) * (nand->cfg->writesz + nand->cfg->metasz);
	rawsz = (nand->cfg->size / nand->cfg->erasesz) * rawEraseBlockSz;
	rawPagesz = nand->cfg->writesz + nand->cfg->metasz;

	if (((len != 0u) && (buff == NULL)) || (offs >= rawsz)) {
		return -EINVAL;
	}

	len = min(len, rawsz - offs);
	if (len == 0u) {
		return 0;
	}

	/* Sync data device changes. */
	err = data_doSync(nand);
	if (err < 0) {
		return err;
	}

	for (ret = 0; ret < len; ret += size) {
		page = (offs + ret) / rawPagesz;
		poffs = (offs + ret) % rawPagesz;
		size = min(len - ret, rawPagesz - poffs);

		err = nanddrv_readraw(nand->dma, page, nand_raw_page, rawPagesz);
		if (err < 0) {
			return -EIO;
		}
		hal_memcpy((u8 *)buff + ret, nand_raw_page + poffs, size);
	}

	return ret;
}


static ssize_t raw_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	nand_t *nand = nand_get(minor);
	size_t size, rawEraseBlockSz, rawsz, rawPagesz, ret;
	int err;

	if (nand == NULL) {
		return -ENODEV;
	}

	rawEraseBlockSz = (nand->cfg->erasesz / nand->cfg->writesz) * (nand->cfg->writesz + nand->cfg->metasz);
	rawsz = (nand->cfg->size / nand->cfg->erasesz) * rawEraseBlockSz;
	rawPagesz = nand->cfg->writesz + nand->cfg->metasz;

	if (((len != 0u) && (buff == NULL)) || (offs >= rawsz) ||
		((rawsz % rawPagesz) != 0u) || ((offs % rawPagesz) != 0u)) {
		return -EINVAL;
	}

	len = min(len, rawsz - offs);
	if (len == 0u) {
		return 0;
	}

	/* Sync data device changes. */
	err = data_doSync(nand);
	if (err < 0) {
		return err;
	}

	for (ret = 0; ret < len; ret += size) {
		size = min(len - ret, rawPagesz);

		hal_memcpy(nand_raw_page, (u8 *)buff + ret, size);
		err = nanddrv_writeraw(nand->dma, (offs + ret) / rawPagesz, nand_raw_page, size);
		if (err < 0) {
			return -EIO;
		}
	}

	return ret;
}


static int raw_sync(unsigned int minor)
{
	// Sync is noop just check if the device exists.
	nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	return 0;
}


static int raw_done(unsigned int minor)
{
	// Done is noop, as finalization is performed in data, just check if the device exists.
	nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	return 0;
}


static int raw_init(unsigned int minor)
{
	nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	lib_printf("\ndev/nand/raw: Configured %s(%d.%d)", nand->cfg->name, DEV_NAND_RAW, minor);

	return 0;
}


__attribute__((constructor)) static void raw_register(void)
{
	static const dev_handler_t h = {
		.read = raw_read,
		.write = raw_write,
		.sync = raw_sync,
		.map = NULL,
		.done = raw_done,
		.init = raw_init,
		.erase = NULL,
	};

	devs_register(DEV_NAND_RAW, NAND_MAX_CNT, &h);
}
