/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB NANDFCTRL2 Data Interface
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


#define NAND_ERASED_STATE 0xffU


static int data_sync(unsigned int minor)
{
	nand_die_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}

	/* Nothing to do */
	return EOK;
}


static ssize_t data_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	nand_die_t *nand = nand_get(minor);
	u32 block, blockcnt, page, rpage;
	addr_t boffs, poffs;
	size_t size, ret = 0;
	int err;

	(void)timeout;

	if (nand == NULL) {
		return -ENODEV;
	}

	if (offs >= nand->info.size) {
		return -EINVAL;
	}

	len = min(len, nand->info.size - offs);

	if (len == 0U) {
		return 0;
	}

	boffs = offs % nand->info.erasesz;
	blockcnt = nand->info.size / nand->info.erasesz;

	/* Read block by block, page by page. */
	for (block = offs / nand->info.erasesz; (block < blockcnt) && (ret < len); block++) {
		/* Skip bad blocks */
		if (nandfctrl2_isbad(nand, block) != 0) {
			continue;
		}

		page = boffs / nand->info.writesz;
		poffs = boffs % nand->info.writesz;

		while ((page < nand->info.pagesPerBlock) && (ret < len)) {
			rpage = block * nand->info.pagesPerBlock + page;
			size = min(len - ret, nand->info.writesz - poffs);

			err = nandfctrl2_pageRead(nand, rpage, nandfctrl2_rawBuffer);
			if (err == -ETIME) {
				(void)nandfctrl2_resetFlash(nand);
				return err;
			}
			if (err < 0) {
				/* Block read failed, return written amount or error */
				return (ret > 0) ? (ssize_t)ret : err;
			}

			(void)hal_memcpy((u8 *)buff + ret, nandfctrl2_rawBuffer + poffs, size);

			ret += size;
			page++;
			poffs = 0;
		}

		boffs = 0;
	}

	return min(ret, len);
}


static ssize_t data_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	nand_die_t *nand = nand_get(minor);
	u32 block, blockcnt, page, wpage;
	addr_t boffs, poffs;
	size_t size, ret = 0;
	int err = 0;

	if (nand == NULL) {
		return -ENODEV;
	}

	if (offs >= nand->info.size) {
		return -EINVAL;
	}

	len = min(len, nand->info.size - offs);

	if (len == 0U) {
		return 0;
	}

	blockcnt = nand->info.size / nand->info.erasesz;
	boffs = offs % nand->info.erasesz;

	for (block = offs / nand->info.erasesz; (block < blockcnt) && (ret < len); block++) {
		/* Skip bad blocks */
		if (nandfctrl2_isbad(nand, block) != 0) {
			continue;
		}

		page = boffs / nand->info.writesz;
		poffs = boffs % nand->info.writesz;

		while ((page < nand->info.pagesPerBlock) && (ret < len)) {
			wpage = block * nand->info.pagesPerBlock + page;
			size = min(len - ret, nand->info.writesz - poffs);

			if ((size == nand->info.writesz) && (poffs == 0U)) {
				/* Full, page-aligned write directly from user buffer */
				err = nandfctrl2_pageWrite(nand, wpage, (const u8 *)buff + ret);
			}
			else {
				/* Partial/Unaligned write: pad with erased state */
				hal_memset(nandfctrl2_rawBuffer, NAND_ERASED_STATE, nand->info.writesz);
				(void)hal_memcpy(nandfctrl2_rawBuffer + poffs, (const u8 *)buff + ret, size);
				err = nandfctrl2_pageWrite(nand, wpage, nandfctrl2_rawBuffer);
			}

			if (err == -ETIME) {
				(void)nandfctrl2_resetFlash(nand);
				return err;
			}
			if (err < 0) {
				/* Write failed, mark as bad block and abort current write sequence */
				if (nandfctrl2_markbad(nand, block) < 0) {
					return -EIO;
				}
				return (ret > 0) ? (ssize_t)ret : err;
			}

			ret += size;
			page++;
			poffs = 0;
		}

		boffs = 0;
	}

	return (ssize_t)ret;
}


static ssize_t data_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	nand_die_t *nand = nand_get(minor);
	u32 block, blockcnt;
	size_t ret = 0;
	int err;

	(void)flags;

	if (nand == NULL) {
		return -ENODEV;
	}

	/* Erase must be block-aligned */
	if ((offs >= nand->info.size) || ((offs % nand->info.erasesz) != 0U) || ((len % nand->info.erasesz) != 0U)) {
		return -EINVAL;
	}

	len = min(len, nand->info.size - offs);
	if (len == 0U) {
		return 0;
	}

	blockcnt = nand->info.size / nand->info.erasesz;

	for (block = offs / nand->info.erasesz; (block < blockcnt) && (ret < len); block++) {
		/* Skip bad blocks */
		if (nandfctrl2_isbad(nand, block) != 0) {
			continue;
		}

		err = nandfctrl2_eraseBlock(nand, block);

		if (err == -ETIME) {
			(void)nandfctrl2_resetFlash(nand);
			return err;
		}

		if (err < 0) {
			/* Flash reported erase failure, mark block as bad */
			if (nandfctrl2_markbad(nand, block) < 0) {
				return -EIO;
			}
		}
		else {
			ret += nand->info.erasesz;
		}
	}

	return (ssize_t)ret;
}


static int data_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	(void)addr;
	(void)sz;
	(void)memaddr;
	(void)memsz;
	(void)a;

	if (minor >= NAND_DIE_CNT) {
		return -EINVAL;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* NAND is not mappable to any region */
	return dev_isNotMappable;
}


static int data_done(unsigned int minor)
{
	return data_sync(minor);
}


static int data_init(unsigned int minor)
{
	int err;
	nand_die_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}

	nand->minor = minor;

	err = nandfctrl2_init(nand);
	if (err < 0) {
		lib_printf("\ndev/flash/nand: (%d.%d) Initialization error: %d", DEV_NAND_DATA, minor, err);
	}
	else {
		lib_printf("\ndev/flash/nand: Configured %s (%d.%d)", nand->info.name, DEV_NAND_DATA, minor);
	}

	return err;
}


__attribute__((constructor)) static void data_register(void)
{
	static const dev_ops_t opsDataNandfctrl2 = {
		.read = data_read,
		.write = data_write,
		.sync = data_sync,
		.map = data_map,
		.erase = data_erase,
	};

	static const dev_t devDataNandfctrl2 = {
		.name = "flash-nandfctrl2-data",
		.init = data_init,
		.done = data_done,
		.ops = &opsDataNandfctrl2,
	};

	devs_register(DEV_NAND_DATA, NAND_DIE_CNT, &devDataNandfctrl2);
}
