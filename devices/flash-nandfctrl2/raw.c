/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB NANDFCTRL2 Raw Interface
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

/* NOTE: raw device relies on data device for init. */


static ssize_t raw_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	nand_die_t *nand = nand_get(minor);
	size_t size, rawEraseBlockSz, rawsz, rawPagesz, ret, page, poffs;
	int err;

	if (nand == NULL) {
		return -ENODEV;
	}

	if (offs >= nand->info.size) {
		return -EINVAL;
	}

	rawPagesz = nand->info.writesz + nand->info.sparesz;
	rawEraseBlockSz = rawPagesz * nand->info.pagesPerBlock;
	rawsz = (nand->info.size / nand->info.erasesz) * rawEraseBlockSz;

	len = min(len, rawsz - offs);
	if (len == 0U) {
		return 0;
	}

	for (ret = 0; ret < len; ret += size) {
		page = (offs + ret) / rawPagesz;
		poffs = (offs + ret) % rawPagesz;
		size = min(len - ret, rawPagesz - poffs);

		err = nandfctrl2_rawPageRead(nand, page, nandfctrl2_rawBuffer);
		if (err < 0) {
			return -EIO;
		}
		hal_memcpy((u8 *)buff + ret, nandfctrl2_rawBuffer + poffs, size);
	}

	return ret;
}


static int raw_sync(unsigned int minor)
{
	nand_die_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	return EOK;
}


static int raw_done(unsigned int minor)
{
	nand_die_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}

	return EOK;
}


static int raw_init(unsigned int minor)
{
	nand_die_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}
	lib_printf("\ndev/nand/raw: Configured %s(%d.%d)", nand->info.name, DEV_NAND_RAW, minor);

	return EOK;
}


__attribute__((constructor)) static void raw_register(void)
{
	static const dev_ops_t opsRawNandfctrl2 = {
		.read = raw_read,
		.write = NULL,
		.sync = raw_sync,
		.map = NULL,
		.erase = NULL,
	};

	static const dev_t devRawNandfctrl2 = {
		.name = "flash-nandfctrl2-raw",
		.init = raw_init,
		.done = raw_done,
		.ops = &opsRawNandfctrl2,
	};

	devs_register(DEV_NAND_RAW, NAND_DIE_CNT, &devRawNandfctrl2);
}
