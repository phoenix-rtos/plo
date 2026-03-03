/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB NANDFCTRL2 meta interface
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>

#include "nandfctrl2.h"

/* NOTE: meta device relies on data device for init. */


static ssize_t meta_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	nand_die_t *nand = nand_get(minor);
	size_t size, ret, totalSparesz;
	u32 page, block;
	int err;

	(void)timeout;

	if (nand == NULL) {
		return -ENODEV;
	}

	totalSparesz = (nand->info.size / nand->info.writesz) * nand->info.spareavail;

	if ((offs >= totalSparesz) || ((offs % nand->info.spareavail) != 0U)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	page = offs / nand->info.spareavail;

	for (ret = 0; ret < len; ret += size) {
		block = page / nand->info.pagesPerBlock;

		/* Skip bad blocks */
		if (nandfctrl2_isbad(nand, block) != 0) {
			page = (block + 1) * nand->info.pagesPerBlock;
			size = 0;
			continue;
		}

		err = flashdrv_metaRead(nand, page, nandfctrl2_rawBuffer, nand->info.spareavail);
		if (err < 0) {
			return -EIO;
		}

		size = min(len - ret, nand->info.spareavail);
		hal_memcpy((u8 *)buff + ret, nandfctrl2_rawBuffer, size);

		page++;
	}

	return (ssize_t)ret;
}


static ssize_t meta_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	nand_die_t *nand = nand_get(minor);
	size_t size, ret, totalSparesz;
	u32 page, block;
	int err;

	if (nand == NULL) {
		return -ENODEV;
	}

	totalSparesz = (nand->info.size / nand->info.writesz) * nand->info.spareavail;

	if ((offs >= totalSparesz) || ((offs % nand->info.spareavail) != 0U)) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	page = offs / nand->info.spareavail;

	for (ret = 0; ret < len; ret += size) {
		block = page / nand->info.pagesPerBlock;

		/* Skip bad blocks - meta is used for writing clean markers */
		if (nandfctrl2_isbad(nand, block) != 0) {
			page = (block + 1) * nand->info.pagesPerBlock;
			size = 0;
			continue;
		}

		size = min(len - ret, nand->info.spareavail);

		hal_memset(nandfctrl2_rawBuffer, 0xff, nand->info.spareavail);
		hal_memcpy(nandfctrl2_rawBuffer, (const u8 *)buff + ret, size);

		err = flashdrv_metaWrite(nand, page, nandfctrl2_rawBuffer, nand->info.spareavail);
		if (err < 0) {
			return -EIO;
		}

		page++;
	}

	return ret;
}


static int meta_sync(unsigned int minor)
{
	/* Sync is noop just check if the device exists */
	nand_die_t *nand = nand_get(minor);

	return (nand == NULL) ? -ENODEV : 0;
}


static int meta_control(unsigned int minor, int cmd, void *args)
{
	nand_die_t *nand = nand_get(minor);
	dev_nandCtrl_t *nandCtrl = args;

	if (nand == NULL) {
		return -ENODEV;
	}

	switch (cmd) {
		case DEV_CONTROL_NAND_ISBAD:
			nandCtrl->arg.isBad.val = nandfctrl2_isbad(nand, nandCtrl->arg.isBad.offs / nand->info.erasesz);
			return EOK;

		default:
			return -EINVAL;
	};
}


static int meta_done(unsigned int minor)
{
	/* Done is noop, finalization is performed in data, just check if the device exists */
	nand_die_t *nand = nand_get(minor);

	return (nand == NULL) ? -ENODEV : 0;
}


static int meta_init(unsigned int minor)
{
	nand_die_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	lib_printf("\ndev/nand/meta: Configured %s(%d.%d)", nand->info.name, DEV_NAND_META, minor);

	return 0;
}


__attribute__((constructor)) static void meta_register(void)
{
	static const dev_ops_t opsMetaNandfctrl2 = {
		.read = meta_read,
		.write = meta_write,
		.sync = meta_sync,
		.control = meta_control,
		.map = NULL,
		.erase = NULL,
	};

	static const dev_t devMetaNandfctrl2 = {
		.name = "flash-nandfctrl2-meta",
		.init = meta_init,
		.done = meta_done,
		.ops = &opsMetaNandfctrl2,
	};

	devs_register(DEV_NAND_META, NAND_DIE_CNT, &devMetaNandfctrl2);
}
