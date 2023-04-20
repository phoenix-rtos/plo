/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL NAND data interface
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


#define NAND_ERASED_STATE 0xff


/* Linker symbols */
extern u8 nand_page[];  /* NAND page cache */
extern u8 nand_block[]; /* NAND eraseblock cache */


static struct {
	nand_t *rnand; /* Last read NAND device */
	nand_t *wnand; /* Last written NAND device */
	u32 rpage;     /* Last read page */
	u32 wblock;    /* Last written eraseblock */
	nanddrv_meta_t meta;
} data_common;


nand_t *nand_get(unsigned minor)
{
	static nand_t nand_dev;

	if (minor < NAND_MAX_CNT) {
		return &nand_dev;
	}
	return NULL;
}


static void data_invalidateNandPage(nand_t *nand)
{
	if (nand == data_common.rnand) {
		data_common.rnand = NULL;
	}
}


int data_doSync(nand_t *nand)
{
	unsigned int i, nblocks, npages;
	int err;

	data_invalidateNandPage(nand);
	if (nand == data_common.wnand) {
		/* Calculate number of device eraseblocks and pages per eraseblock */
		nblocks = nand->cfg->size / nand->cfg->erasesz;
		npages = nand->cfg->erasesz / nand->cfg->writesz;

		do {
			/* The cached block might have become a bad block */
			/* Find first good block to write the cached data */
			while ((data_common.wblock < nblocks) && nanddrv_isbad(nand->dma, data_common.wblock * npages)) {
				data_common.wblock++;
			}

			/* Fatal error, no good block available */
			if (data_common.wblock >= nblocks) {
				return -ENOSPC;
			}

			/* Erase and write the block */
			err = nanddrv_erase(nand->dma, data_common.wblock * npages);
			if (err >= 0) {
				for (i = 0; i < npages; i++) {
					err = nanddrv_write(nand->dma, (data_common.wblock * npages) + i, nand_block + (i * nand->cfg->writesz), NULL);
					if (err < 0) {
						break;
					}
				}
			}

			/* Block sync failed, mark it as bad and try to sync again on the next block */
			if (err < 0) {
				/* Fatal error, can't recover */
				if (nanddrv_markbad(nand->dma, data_common.wblock * npages) < 0) {
					return -EIO;
				}
				data_common.wblock++;
			}
		} while (err < 0);

		/* Mark cache empty */
		data_common.wnand = NULL;
	}

	return EOK;
}


static int data_sync(unsigned int minor)
{
	nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}

	return data_doSync(nand);
}


static ssize_t data_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	nand_t *nand = nand_get(minor);
	unsigned int block, nblocks, npages, page;
	addr_t boffs, poffs;
	size_t size, ret = 0;
	int err;

	(void)timeout;

	if (nand == NULL) {
		return -ENODEV;
	}

	if (((len != 0u) && (buff == NULL)) || (offs >= nand->cfg->size)) {
		return -EINVAL;
	}

	len = min(len, nand->cfg->size - offs);

	if (len == 0u) {
		return 0;
	}

	nblocks = nand->cfg->size / nand->cfg->erasesz;
	npages = nand->cfg->erasesz / nand->cfg->writesz;
	boffs = offs % nand->cfg->erasesz;

	/* Read block by block. */
	for (block = offs / nand->cfg->erasesz; (block < nblocks) && (ret < len); block++) {
		/* Skip bad blocks */
		if (nanddrv_isbad(nand->dma, block * npages) != 0) {
			continue;
		}

		if ((nand == data_common.wnand) && (block == data_common.wblock)) {
			size = min(len - ret, nand->cfg->erasesz - boffs);
			hal_memcpy((u8 *)buff + ret, nand_block + boffs, size);
			ret += size;
		}
		else {
			poffs = (boffs % nand->cfg->writesz);

			/* Read pages from block. */
			for (page = (block * npages) + (boffs / nand->cfg->writesz); (page < ((block + 1u) * npages)) && (ret < len); page++) {
				/* New cached page */
				if ((nand != data_common.rnand) || (page != data_common.rpage)) {
					err = nanddrv_read(nand->dma, page, nand_page, &data_common.meta);
					/* Block read failed (block data is lost), mark it as bad and exit*/
					if (err < 0) {
						/* Mark page cache empty */
						data_common.rnand = NULL;

						(void)nanddrv_markbad(nand->dma, block * npages);

						return -EIO;
					}

					/* Set new cached page */
					data_common.rnand = nand;
					data_common.rpage = page;
				}

				/* Copy data to buffer */
				size = min(len - ret, nand->cfg->writesz - poffs);
				hal_memcpy((u8 *)buff + ret, nand_page + poffs, size);
				ret += size;
				poffs = 0;
			}
		}

		boffs = 0;
	}

	return min(ret, len);
}


static ssize_t data_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	nand_t *cnand, *nand = nand_get(minor);
	unsigned int i, cblock, block, nblocks, npages;
	addr_t boffs;
	size_t size, ret = 0;
	int err = 0;

	if (nand == NULL) {
		return -ENODEV;
	}

	if (((len != 0u) && (buff == NULL)) || (offs >= nand->cfg->size)) {
		return -EINVAL;
	}

	len = min(len, nand->cfg->size - offs);

	if (len == 0u) {
		return 0;
	}

	data_invalidateNandPage(nand);

	nblocks = nand->cfg->size / nand->cfg->erasesz;
	npages = nand->cfg->erasesz / nand->cfg->writesz;
	boffs = offs % nand->cfg->erasesz;

	for (block = offs / nand->cfg->erasesz; (block < nblocks) && (ret < len); block++) {
		/* Skip bad blocks */
		if (nanddrv_isbad(nand->dma, block * npages) != 0) {
			continue;
		}

		/* New cached block */
		if ((nand != data_common.wnand) || (block != data_common.wblock)) {
			/* Cached block */
			cnand = data_common.wnand;
			cblock = data_common.wblock;

			/* Sync cached block */
			if (cnand != NULL) {
				err = data_doSync(cnand);
				/* Fatal error, can't sync cached block */
				if (err < 0) {
					return err;
				}

				/* Cached data got synced at our block, find next good block */
				if ((nand == cnand) && (block > cblock) && (block <= data_common.wblock)) {
					block = data_common.wblock;
					continue;
				}
			}

			/* Read new cached block data in front of the written area */
			for (i = 0; (i * nand->cfg->writesz) < boffs; i++) {
				err = nanddrv_read(nand->dma, (block * npages) + i, nand_block + (i * nand->cfg->writesz), &data_common.meta);
				if (err < 0) {
					break;
				}
			}

			if (err >= 0) {
				/* Read new cached block data behind the written area */
				for (i = max(i, (boffs + len - ret) / nand->cfg->writesz); i < npages; i++) {
					err = nanddrv_read(nand->dma, (block * npages) + i, nand_block + (i * nand->cfg->writesz), &data_common.meta);
					if (err < 0) {
						break;
					}
				}
			}

			/* Block read failed (block data is lost), mark it as bad and find next good block */
			if (err < 0) {
				/* Fatal error, can't recover */
				if (nanddrv_markbad(nand->dma, block * npages) < 0) {
					return -EIO;
				}
				continue;
			}

			/* Set new cached block */
			data_common.wnand = nand;
			data_common.wblock = block;
		}

		/* Copy data to cache */
		size = min(len - ret, nand->cfg->erasesz - boffs);
		hal_memcpy(nand_block + boffs, (const u8 *)buff + ret, size);
		ret += size;
		boffs = 0;
	}

	return min(ret, len);
}


static ssize_t data_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	nand_t *nand = nand_get(minor);
	size_t boffs, size, i, block, nblocks, npages, page, ret = 0;
	int err;

	(void)flags;

	if (nand == NULL) {
		return -ENODEV;
	}

	if (offs >= nand->cfg->size) {
		return -EINVAL;
	}

	len = min(len, nand->cfg->size - offs);
	if (len == 0u) {
		return 0;
	}

	data_invalidateNandPage(nand);

	npages = nand->cfg->erasesz / nand->cfg->writesz;
	nblocks = nand->cfg->size / nand->cfg->erasesz;
	boffs = offs % nand->cfg->erasesz;

	for (block = offs / nand->cfg->erasesz; (block < nblocks) && (ret < len); block++) {
		page = block * npages;
		/* Skip bad blocks */
		if (nanddrv_isbad(nand->dma, page) != 0) {
			continue;
		}

		if ((offs != 0u) || ((ret + nand->cfg->erasesz) > len)) {
			/* Partial block erase */

			/* Check if the block is in the cache. */
			if ((data_common.wnand != nand) || (data_common.wblock != block)) {
				if (data_common.wnand != NULL) {
					err = data_doSync(data_common.wnand);
					/* Fatal error, can't sync cached block */
					if (err < 0) {
						return err;
					}
				}

				/* Read new cached block data behind the written area */
				for (i = 0; i < npages; i++) {
					err = nanddrv_read(nand->dma, page + i, nand_block + (i * nand->cfg->writesz), &data_common.meta);
					if (err < 0) {
						break;
					}
				}

				if (err < 0) {
					err = nanddrv_markbad(nand->dma, page);
					if (err < 0) {
						return -EIO;
					}
					continue;
				}

				data_common.wnand = nand;
				data_common.wblock = block;
			}

			size = min(len - ret, nand->cfg->erasesz - boffs);
			hal_memset(nand_block + boffs, NAND_ERASED_STATE, size);
			ret += size;
			boffs = 0;
		}
		else {
			/* Full block erase */

			if ((data_common.wnand == nand) && (data_common.wblock == block)) {
				/* Invalidate cache. */
				data_common.wnand = NULL;
			}

			err = nanddrv_erase(nand->dma, page);
			if (err < 0) {
				err = nanddrv_markbad(nand->dma, page);
				if (err < 0) {
					return -EIO;
				}
			}
			else {
				ret += nand->cfg->erasesz;
			}
		}
	}

	return ret;
}


static int data_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	(void)sz;
	(void)memaddr;
	(void)addr;
	(void)memsz;
	(void)a;

	if (minor >= NAND_MAX_CNT) {
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
	nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}

	nanddrv_init();

	nand->cfg = nanddrv_info();
	nand->dma = nanddrv_dma();

	lib_printf("\ndev/flash/nand: Configured %s(%d.%d)", nand->cfg->name, DEV_NAND_DATA, minor);

	return 0;
}


__attribute__((constructor)) static void data_register(void)
{
	static const dev_handler_t h = {
		.read = data_read,
		.write = data_write,
		.sync = data_sync,
		.map = data_map,
		.done = data_done,
		.init = data_init,
		.erase = data_erase,
	};

	devs_register(DEV_NAND_DATA, NAND_MAX_CNT, &h);
}
