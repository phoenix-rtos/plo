/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Disk routines
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2001, 2005, 2006 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "config.h"
#include "errors.h"
#include "low.h"
#include "plostd.h"
#include "timer.h"
#include "serial.h"
#include "disk.h"


s32 disk_open(u16 dn, char *name, u32 flags)
{
	return 64;
}


/* Function reads bytes from file specified by starting block number (handle) */
s32 disk_read(u16 dn, s32 handle, u32 *pos, u8 *buff, u32 len)
{
	static u8 lh, loffs, empty = 1;
	static u16 ldn, lc;
	char *cache = RCACHE_OFFS;
	u8 h, s, heads, secs, offs;
	u16 c, cyls;
	u32 sb, eb, l = 0;
	u16 boffs, size;
	int err;

	if (!len)
		return ERR_ARG;

	/* Get disk geometry */
	if (err = low_int13param(dn, &cyls, &heads, &secs))
		return ERR_PHFS_IO;

	cyls += 1;
	heads += 1;

	/* Calculate start and end block numbers */
	sb = handle + *pos / SECTOR_SIZE;
	eb = handle + (*pos + len - 1) / SECTOR_SIZE;

	for (; sb <= eb; sb++) {
		c = sb / secs / heads;
		h = (sb / secs) % heads;
		s = 1 + (sb % secs);

		offs = 1 + ((s - 1) / RCACHE_SIZE) * RCACHE_SIZE;
		boffs = *pos % SECTOR_SIZE;

		/* Read track from disk to cache */
		if (empty || (dn != ldn) || (c != lc) || (h != lh) || (offs != loffs)) {
			if ((err = low_int13access(INT13_READ, dn, c, h, offs, min(RCACHE_SIZE, secs), cache)))
				return ERR_PHFS_IO;

			ldn = dn;
			lc = c;
			lh = h;
			loffs = offs;
			empty = 0;
		}

		/* Read data from cache */
		size = (sb == eb) ? len - l : SECTOR_SIZE - boffs;
		low_memcpy(buff, cache + (s - 1) % RCACHE_SIZE * SECTOR_SIZE + boffs, size);
		buff += size;
		*pos += size;
		l += size;
	}

	return l;
}


/* Function writes bytes from file specified by starting block number (handle) */
s32 disk_write(u16 dn, s32 handle, u32 *pos, u8 *buff, u32 len, u8 sync)
{
	static u8 lh, loffs, lb = 0, le = 0;
	static u16 ldn, lc;
	char *cache = WCACHE_OFFS;
	u8 b, h, s, heads, secs, offs;
	u16 c, cyls;
	u32 sb, eb, l = 0;
	u16 boffs, size;
	int err;

	if (!len && !sync)
		return ERR_ARG;

	if (len) {
		/* Get disk geometry */
		if (err = low_int13param(dn, &cyls, &heads, &secs))
			return ERR_PHFS_IO;

		cyls += 1;
		heads += 1;

		/* Calculate start and end block numbers */
		sb = handle + *pos / SECTOR_SIZE;
		eb = handle + (*pos + len - 1) / SECTOR_SIZE;

		for (; sb <= eb; sb++) {
			c = sb / secs / heads;
			h = (sb / secs) % heads;
			s = 1 + (sb % secs);

			offs = 1 + ((s - 1) / WCACHE_SIZE) * WCACHE_SIZE;
			boffs = *pos % SECTOR_SIZE;
			b = (s - 1) % WCACHE_SIZE;

			/* Write compact track segment from cache to disk */
			if ((lb != le) && ((dn != ldn) || (c != lc) || (h != lh) || (offs != loffs) || (b + 1 < lb) || (b > le))) {
				if ((err = low_int13access(INT13_WRITE, dn, lc, lh, loffs + lb, le - lb, cache + lb * SECTOR_SIZE)))
					return ERR_PHFS_IO;

				/* Mark cache empty */
				lb = le = 0;
			}

			/* Write data to cache */
			size = (sb == eb) ? len - l : SECTOR_SIZE - boffs;
			low_memcpy(cache + b * SECTOR_SIZE + boffs, buff, size);
			buff += size;
			*pos += size;
			l += size;

			/* Update cache info */
			ldn = dn;
			lc = c;
			lh = h;
			loffs = offs;

			if (lb == le) {
				lb = b;
				le = b + 1;
			}
			else if (b + 1 == lb) {
				lb--;
			}
			else if (b == le) {
				le++;
			}
		}
	}

	if (sync && (lb != le)) {
		if ((err = low_int13access(INT13_WRITE, ldn, lc, lh, loffs + lb, le - lb, cache + lb * SECTOR_SIZE)))
			return ERR_PHFS_IO;

		lb = le = 0;
	}

	return l;
}


s32 disk_close(u16 dn, s32 handle)
{
	return disk_write(dn, 0L, NULL, NULL, 0UL, 1);
}
