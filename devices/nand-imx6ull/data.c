/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL NAND data interface
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <devices/devs.h>

#include "nand.h"


/* Linker symbols */
extern u8 nand_page[];  /* NAND page cache */
extern u8 nand_block[]; /* NAND eraseblock cache */


static struct {
	const nand_t *rnand; /* Last read NAND device */
	const nand_t *wnand; /* Last written NAND device */
	unsigned int rpage;  /* Last read page */
	unsigned int wblock; /* Last written eraseblock */
} data_common;


static int _data_sync(const nand_t *nand)
{
	unsigned int i, nblocks, npages;
	int err;

	if (nand == data_common.wnand) {
		/* Calculate number of device eraseblocks and pages per eraseblock */
		nblocks = nand->cfg->size / nand->cfg->erasesz;
		npages = nand->cfg->erasesz / nand->cfg->pagesz;

		do {
			/* The cached block might have become a bad block */
			/* Find first good block to write the cached data */
			while ((data_common.wblock < nblocks) && nand_isBad(nand, data_common.wblock)) {
				data_common.wblock++;
			}

			/* Fatal error, no good block available */
			if (data_common.wblock >= nblocks) {
				return -ENOSPC;
			}

			/* Erase and write the block */
			err = nand_erase(nand, data_common.wblock);
			if (err >= 0) {
				for (i = 0; i < npages; i++) {
					err = nand_write(nand, data_common.wblock * npages + i, nand_block + i * nand->cfg->pagesz, NULL, 0);
					if (err < 0) {
						break;
					}
				}
			}

			/* Block sync failed, mark it as bad and try to sync again on the next block */
			if (err < 0) {
				/* Fatal error, can't recover */
				if (nand_markBad(nand, data_common.wblock) < 0) {
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
	const nand_t *nand = nand_get(minor);

	if (nand == NULL) {
		return -ENODEV;
	}

	return _data_sync(nand);
}


static ssize_t data_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	const nand_t *nand = nand_get(minor);
	unsigned int i, page, block, nblocks, npages;
	size_t size, bret, ret = 0;
	int err = EOK;
	u8 *data;

	if (nand == NULL) {
		return -ENODEV;
	}

	if ((buff == NULL) || (offs > nand->cfg->size)) {
		return -EINVAL;
	}

	if (len > nand->cfg->size - offs) {
		len = nand->cfg->size - offs;
	}

	if (len == 0) {
		return 0;
	}

	nblocks = nand->cfg->size / nand->cfg->erasesz;
	npages = nand->cfg->erasesz / nand->cfg->pagesz;
	block = offs / nand->cfg->erasesz;
	page = offs / nand->cfg->pagesz;
	offs %= nand->cfg->pagesz;

	while (ret < len) {
		/* Skip bad blocks */
		while ((block < nblocks) && nand_isBad(nand, block)) {
			page += npages;
			block++;
		}

		/* No good block left to read from */
		if (block >= nblocks) {
			return ret;
		}

		/* Read block pages */
		bret = 0;
		for (i = page - block * npages; i < npages; i++) {
			/* Check cached block */
			if ((nand == data_common.wnand) && (block == data_common.wblock)) {
				data = nand_block + i * nand->cfg->pagesz;
			}
			/* Check cached page */
			else {
				/* New cached page */
				if ((nand != data_common.rnand) || (page != data_common.rpage)) {
					err = nand_read(nand, page, nand_page, nand_page + nand->cfg->pagesz, 0);
					/* Block read failed (block data is lost), mark it as bad and read next one */
					if (err < 0) {
						/* Mark page cache empty */
						data_common.rnand = NULL;

						/* Fatal error, can't recover */
						if (nand_markBad(nand, block) < 0) {
							return -EIO;
						}

						/* Discard data read from bad block and proceed to read the next one */
						block++;
						page = block * npages;
						bret = 0;
						break;
					}

					/* Set new cached page */
					data_common.rnand = nand;
					data_common.rpage = page;
				}

				data = nand_page;
			}

			/* Copy data to buffer */
			if (err >= 0) {
				size = min(len - ret, nand->cfg->pagesz - offs);
				hal_memcpy((u8 *)buff + ret, data + offs, size);
				bret += size;
				offs = 0;
			}
		}

		ret += bret;
	}

	return ret;
}


static ssize_t data_write(unsigned int minor, addr_t offs, const void *buff, size_t len);
{
	const nand_t *cnand, *nand = nand_get(minor);
	unsigned int i, cblock, block, nblocks, npages;
	size_t size, ret = 0;
	int err;

	if (nand == NULL) {
		return -ENODEV;
	}

	if ((buff == NULL) || (offs > nand->cfg->size)) {
		return -EINVAL;
	}

	if (len > nand->cfg->size - offs) {
		len = nand->cfg->size - offs;
	}

	if (len == 0) {
		return 0;
	}

	nblocks = nand->cfg->size / nand->cfg->erasesz;
	npages = nand->cfg->erasesz / nand->cfg->pagesz;
	block = offs / nand->cfg->erasesz;
	offs %= nand->cfg->erasesz;

	while (ret < len) {
		/* Skip bad blocks */
		while ((block < nblocks) && nand_isBad(nand, block)) {
			block++;
		}

		/* No good block left to write to */
		if (block >= nblocks) {
			return ret;
		}

		/* New cached block */
		if ((nand != data_common.wnand) || (block != data_common.wblock)) {
			/* Cached block */
			cnand = data_common.wnand;
			cblock = data_common.wblock;

			/* Sync cached block */
			if (cnand != NULL) {
				err = _data_sync(cnand);
				/* Fatal error, can't sync cached block */
				if (err < 0) {
					return err;
				}

				/* Cached data got synced at our block, find next good block */
				if ((nand == cnand) && (block > cblock) && (block <= data_common.wblock)) {
					block = data_common.wblock + 1;
					continue;
				}
			}

			/* Read new cached block data in front of the written area */
			for (i = 0; i * nand->cfg->pagesz < offs; i++) {
				err = nand_read(nand, block * npages + i, nand_block + i * nand->cfg->pagesz, nand_page, 0);
				if (err < 0) {
					break;
				}
			}

			if (err >= 0) {
				/* Read new cached block data behind the written area */
				for (i = max(i, (offs + len - ret) / nand->cfg->pagesz); i < npages; i++) {
					err = nand_read(nand, block * npages + i, nand_block + i * nand->cfg->pagesz, nand_page, 0);
					if (err < 0) {
						break;
					}
				}
			}

			/* Block read failed (block data is lost), mark it as bad and find next good block */
			if (err < 0) {
				/* Fatal error, can't recover */
				if (nand_markBad(nand, block) < 0) {
					return -EIO;
				}
				block++;
				continue;
			}

			/* Set new cached block */
			data_common.wnand = nand;
			data_common.wblock = block;
		}

		/* Copy data to cache */
		size = min(len - ret, nand->cfg->erasesz - offs);
		hal_memcpy(nand_block + offs, (const u8 *)buff + ret, size);
		ret += size;
		offs = 0;
		block++;
	}

	return ret;
}


static int data_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
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
	data_sync(minor);

	return nand_done(minor);
}


static int data_init(unsigned int minor)
{
	/* Mark caches empty */
	data_common.rnand = NULL;
	data_common.wnand = NULL;

	return nand_init(minor);
}


__attribute__((constructor)) static void data_register(void)
{
	static const dev_handler_t h[] = {
		.read = data_read,
		.write = data_write,
		.sync = data_sync,
		.map = data_map,
		.done = data_done,
		.init = data_init,
	};

	devs_register(DEV_NAND_DATA, NAND_MAX_CNT, &h);
}
