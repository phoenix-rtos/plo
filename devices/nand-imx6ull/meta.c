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

/* NOTICE: meta device relies on data device for init. */


static ssize_t meta_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	static nanddrv_meta_t meta;
	nand_t *nand = nand_get(minor);
	size_t size, ret, metasz, page;
	int err;

	(void)timeout;

	if (nand == NULL) {
		return -ENODEV;
	}

	metasz = (nand->cfg->size / nand->cfg->erasesz) * nand->cfg->oobsz;

	if (((len != 0u) && (buff == NULL)) ||
		(offs >= metasz) || ((offs % nand->cfg->writesz) != 0u)) {
		return -EINVAL;
	}

	len = min(len, metasz - offs);
	if (len == 0u) {
		return 0;
	}

	page = offs / nand->cfg->writesz;

	/* Sync data device changes. */
	err = data_doSync(nand);
	if (err < 0) {
		return err;
	}

	for (ret = 0; ret < len; ret += size) {
		err = nanddrv_read(nand->dma, page, NULL, &meta);
		if (err < 0) {
			return -EIO;
		}

		size = min(len - ret, nand->cfg->oobsz);
		hal_memcpy((u8 *)buff + ret, meta.metadata, size);
		page++;
	}


	return ret;
}


static ssize_t meta_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	static char metaBuf[64];
	nand_t *nand = nand_get(minor);
	size_t size, ret, metasz, page;
	int err;

	if (nand == NULL) {
		return -ENODEV;
	}

	metasz = (nand->cfg->size / nand->cfg->erasesz) * nand->cfg->oobsz;

	if (((len != 0u) && (buff == NULL)) ||
		(offs >= metasz) || ((offs % nand->cfg->writesz) != 0u)) {
		return -EINVAL;
	}

	len = min(len, metasz - offs);
	if (len == 0u) {
		return 0;
	}

	page = offs / nand->cfg->writesz;

	/* Sync data device changes. */
	err = data_doSync(nand);
	if (err < 0) {
		return err;
	}

	for (ret = 0; ret < len; ret += size) {
		/* No bad blocks skipping as meta is used to mark blocks as bad. */
		size = min(len - ret, nand->cfg->oobsz);

		hal_memset(metaBuf, 0xff, nand->cfg->oobsz);
		hal_memcpy(metaBuf, (u8 *)buff + ret, size);

		err = nanddrv_write(nand->dma, page, NULL, metaBuf);
		if (err < 0) {
			return -EIO;
		}

		page++;
	}

	return ret;
}


static int meta_sync(unsigned int minor)
{
	// Sync is noop just check if the device exists.
	nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	return 0;
}


static int meta_done(unsigned int minor)
{
	// Done is noop, as finalization is performed in data, just check if the device exists.
	nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	return 0;
}


static int meta_init(unsigned int minor)
{
	nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	lib_printf("\ndev/nand/meta: Configured %s(%d.%d)", nand->cfg->name, DEV_NAND_META, minor);

	return 0;
}


__attribute__((constructor)) static void meta_register(void)
{
	static const dev_handler_t h = {
		.read = meta_read,
		.write = meta_write,
		.sync = meta_sync,
		.map = NULL,
		.done = meta_done,
		.init = meta_init,
		.erase = NULL,
	};

	devs_register(DEV_NAND_META, NAND_MAX_CNT, &h);
}
