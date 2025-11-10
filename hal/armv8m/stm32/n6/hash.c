/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32N6 Hash processor driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hash.h"
#include <lib/errno.h>
#include <lib/lib.h>

static struct {
	volatile u32 *base;
	u32 blocksize; /* Blocksize of ongoing digest operation. 0 means operation finished */
	u32 written;   /* Total written bytes to last block */
	u32 leftover;      /* Leftover bytes from message that couldn't be written to fifo */
	u32 leftoverCount; /* Number of leftover bytes */
} common;


static u32 hash_getBlockSize(u32 algo)
{
	u32 blocksize;

	*(common.base + hash_cr) = algo | HASH_CR_INIT;
	hal_cpuDataMemoryBarrier();
	/* SR_NBWE contains the number N + 1, where N is the number of 32-bit words that make up a block.
	 * When block size is 128 bytes, SR_NBWE is 32+1==33. Then the 21-st bit of HASH_SR is set, despite it being resereved in the Reference Manual.
	 * This seems to be an error in documentation. Avoid reading this bit just in case. */
	blocksize = (((*(common.base + hash_sr) >> HASH_SR_NBWE_OFF) & 0x1f) - 1) * 4;
	hal_cpuDataMemoryBarrier();

	/* blocksize is 0 because we read 0x1 instead of 0x21. Set it manually to 128 bytes */
	if (blocksize == 0) {
		blocksize = 128;
	}
	return blocksize;
}


/* Output ends up being in little endian */
static void hash_copyDigest(u8 *digest, u32 digestbytes)
{
	if (digest == NULL) {
		return;
	}

	u32 reg = 0;
	u32 off = digestbytes / 4;
	u32 *dp = (u32 *)digest;
	u32 t;

	do {
		t = *(common.base + hash_hr0 + reg);
		dp[--off] = t;
		reg++;
	} while (off > 0);
}


/* Begin a digest operation */
int hash_initDigest(u32 algo)
{
	if (common.blocksize != 0) {
		return -EINVAL;
	}

	common.blocksize = hash_getBlockSize(algo);
	common.written = 0;
	common.leftoverCount = 0;
	common.leftover = 0;

	/* HASH accepts data in big endian. Enable per byte data swapping */
	*(common.base + hash_cr) = algo | HASH_CR_SWAP_BYTES;
	hal_cpuDataMemoryBarrier();
	*(common.base + hash_cr) |= HASH_CR_INIT;
	hal_cpuDataMemoryBarrier();

	return EOK;
}


/* Feed data to an ongoing digest operation */
int hash_feedMessage(const u8 *data, u32 bytes)
{
	if (common.blocksize == 0) {
		return -EINVAL;
	}

	/* Check if last feed was unaligned and if so, fix it */
	if (common.leftoverCount != 0) {
		/* Align the leftovers to 4 bytes */
		while (bytes > 0 && common.leftoverCount < 4) {
			common.leftover |= (*data) << (8 * common.leftoverCount);
			data++;
			bytes--;
			common.leftoverCount++;
		}
		if (common.leftoverCount == 4) {
			*(common.base + hash_din) = common.leftover;
			hal_cpuDataMemoryBarrier();
			common.written += 4;
			common.leftover = 0;
			common.leftoverCount = 0;
		}
	}

	if (common.written > common.blocksize) {
		while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
			; /* Wait for partial digest computation */
		}
		common.written -= common.blocksize;
	}

	while (bytes >= 4) {
		*(common.base + hash_din) = *((const u32 *)(data));
		hal_cpuDataMemoryBarrier();
		data += 4;
		common.written += 4;
		bytes -= 4;

		if (common.written > common.blocksize) {
			while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
				; /* Wait for partial digest computation */
			}
			common.written -= common.blocksize;
		}
	}

	/* Put remaining bytes in leftover buffer. Leftover must be 0 here */
	if (bytes > 0) {
		common.leftover = 0;
		common.leftoverCount = 0;
		while (bytes > 0 && common.leftoverCount < 4) {
			common.leftover |= (*data) << (common.leftoverCount * 8);
			data++;
			bytes--;
			common.leftoverCount++;
		}
	}

	return EOK;
}


int hash_getDigest(u8 *out, u32 outbytes)
{
	u32 t;

	if (common.blocksize == 0) {
		return -EINVAL;
	}

	if (common.leftoverCount != 0) {
		*(common.base + hash_din) = common.leftover;
		hal_cpuDataMemoryBarrier();
	}

	t = *(common.base + hash_str) & ~(HASH_STR_NBLW_MASK);
	t |= (common.leftoverCount * 8) % 32;
	*(common.base + hash_str) = t;
	hal_cpuDataMemoryBarrier();

	*(common.base + hash_str) |= HASH_STR_DCAL;
	hal_cpuDataMemoryBarrier();

	while ((*(common.base + hash_sr) & HASH_SR_DCIS) == 0) {
		; /* Wait for digest calculation to complete */
	}

	hash_copyDigest(out, outbytes);

	common.blocksize = 0;
	common.written = 0;
	common.leftover = 0;
	common.leftoverCount = 0;

	return EOK;
}


/* Calculate hash digest in a single function call. Message length counted in bytes. Output digest in little endian.
 * HASH peripheral supports message sizes measured in bits, but we don't need that kind of granularity.
 * User has to provide bith algorithm type and the number of bytes of the output hash. */
int hash_digest(u32 algo, const u8 *mess, u32 mbytes, u8 *out, u32 outbytes)
{
	/* Assume correct arguments */
	hash_initDigest(algo);
	hash_feedMessage(mess, mbytes);
	hash_getDigest(out, outbytes);

	return EOK;
}


void hash_cancelOperation()
{
	hash_getDigest(NULL, 0);
}


void hash_init(void)
{
	common.base = HASH_BASE;
	common.blocksize = 0;

	/* BOOT ROM can use this peripheral, so it's better to reset it. */
	_stm32_rccSetDevClock(dev_hash, 0);
	_stm32_rccDevReset(dev_hash, 1);
	_stm32_rccDevReset(dev_hash, 0);
	_stm32_rccSetDevClock(dev_hash, 1);

	return;
}
