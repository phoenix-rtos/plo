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
 * %LICENSE%
 */

#include "hash.h"
#include <lib/errno.h>
#include <lib/lib.h>

static struct {
	volatile u32 *base;
	u32 currBlocksize; /* Blocksize of ongoing digest operation. 0 means operation finished */
	u32 currWritten;   /* Total written bytes to last block */
	u32 leftover;      /* Leftover bytes that couldn't be written to fifo */
	u32 leftoverAmount; /* Number of bytes that couldn't be written to fifo */
} common;


static u32 hash_getBlockSize(u32 algo)
{
	u32 blocksize;

	*(common.base + hash_cr) = algo | HASH_CR_INIT;
	hal_cpuDataMemoryBarrier();
	/* SR_NBWE contains the number N + 1, where N is the number of 32bit words that make up a block.
	 * If SR_NBWE is 32+1==33 (blocksize of 128 bits) then SR_NBWE carries over into bit 21 of HASH_SR which is
	 * said to be reserved by the Reference Manual.
	 * This seems to be an error in documentation, but we don't read it just in case. */
	blocksize = (((*(common.base + hash_sr) >> HASH_SR_NBWE_OFF) & 0x1f) - 1) * 4;
	hal_cpuDataMemoryBarrier();

	/* blocksize is 0 because we read 0x1 instead of 0x21. Set it manually to 128 */
	if (blocksize == 0) {
		blocksize = 128;
	}
	return blocksize;
}


/* Output ends up being in little endian */
static void hash_copyDigest(u8 *digest, u32 digestbytes)
{
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
	// u32 t;

	if (common.currBlocksize != 0) {
		return -EINVAL;
	}

	common.currBlocksize = hash_getBlockSize(algo);
	common.currWritten = 0;
	common.leftoverAmount = 0;
	common.leftover = 0;

	lib_printf("blocksize: %d\n", common.currBlocksize);

	// t = *(common.base + hash_str) & ~(HASH_STR_NBLW_MASK);
	// t |= (3 * 8) % 32;
	// *(common.base + hash_str) = t;
	// hal_cpuDataMemoryBarrier();

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
	u32 off = 0;
	u32 t, toff, i;

	if (common.currBlocksize == 0) {
		return -EINVAL;
	}

	/* Check if last feed was unaligned and if so, fix it */
	if (common.leftoverAmount != 0) {
		t = 4 - common.leftoverAmount; /* Bytes to add to leftover buffer */
		if (t > bytes) {
			t = bytes;
		}
		bytes -= t;
		while (off < t) {
			i = common.leftoverAmount;
			common.leftover |= (*(data + off)) << (8 * i);
			off++;
			common.leftoverAmount++;
		}
		if (common.leftoverAmount == 4) {
			*(common.base + hash_din) = common.leftover;
			hal_cpuDataMemoryBarrier();
			common.currWritten += 4;
			common.leftover = 0;
			common.leftoverAmount = 0;
		}
	}

	if (common.currWritten > common.currBlocksize) {
		while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
			; /* Wait for partial digest computation */
		}
		common.currWritten -= common.currBlocksize;
	}

	while (bytes >= 4) {
		*(common.base + hash_din) = *((const u32 *)(data + off));
		hal_cpuDataMemoryBarrier();
		off += 4;
		common.currWritten += 4;
		bytes -= 4;

		if (common.currWritten > common.currBlocksize) {
			while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
				; /* Wait for partial digest computation */
			}
			common.currWritten -= common.currBlocksize;
		}
	}

	/* Put remaining bytes in leftover buffer */
	if (bytes > 0) {
		lib_printf("tutaj\n");
		toff = 0;
		t = 0;
		while (toff < bytes) {
			t |= (*(data + off) << (toff * 8));
			toff += 1;
			off += 1;
		}
		common.leftover = t;
		common.leftoverAmount = bytes;
		common.currWritten += bytes;

		if (common.currWritten > common.currBlocksize) {
			while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
				; /* Wait for partial digest computation */
			}
			common.currWritten -= common.currBlocksize;
		}
	}

	return EOK;
}


int hash_getDigest(u8 *out, u32 outbytes)
{
	u32 t;

	if (common.currBlocksize == 0) {
		return -EINVAL;
	}

	if (common.leftoverAmount != 0) {
		lib_printf("leftover: %08x\n", common.leftover);
		*(common.base + hash_din) = common.leftover;
		hal_cpuDataMemoryBarrier();
	}

	t = *(common.base + hash_str) & ~(HASH_STR_NBLW_MASK);
	t |= (common.leftoverAmount * 8) % 32;
	*(common.base + hash_str) = t;
	hal_cpuDataMemoryBarrier();

	*(common.base + hash_str) |= HASH_STR_DCAL;
	hal_cpuDataMemoryBarrier();

	while ((*(common.base + hash_sr) & HASH_SR_DCIS) == 0) {
		; /* Wait for digest calculation to complete */
	}

	hash_copyDigest(out, outbytes);

	common.currBlocksize = 0;
	common.currWritten = 0;
	common.leftover = 0;
	common.leftoverAmount = 0;

	return EOK;
}


// static void hash_process(const u8 *mess, u32 mbytes, u32 blocksize)
// {
// 	u32 off = 0;
// 	u32 written = 0;
// 	u32 t, toff;

// 	while (mbytes >= 4) {
// 		*(common.base + hash_din) = *((const u32 *)(mess + off));
// 		hal_cpuDataMemoryBarrier();
// 		off += 4;
// 		written += 4;
// 		mbytes -= 4;

// 		if (written > blocksize) {
// 			while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
// 				; /* Wait for partial digest computation */
// 			}
// 			written -= blocksize;
// 		}
// 	}
// 	if (mbytes > 0) {
// 		toff = 0;
// 		t = 0;
// 		while (toff < mbytes) {
// 			t |= (*(mess + off) << (toff * 8));
// 			toff += 1;
// 			off += 1;
// 		}
// 		*(common.base + hash_din) = t;
// 		hal_cpuDataMemoryBarrier();
// 		written += mbytes;

// 		if (written > blocksize) {
// 			while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
// 				; /* Wait for partial digest computation */
// 			}
// 			written -= blocksize;
// 		}

// 		t = *(common.base + hash_str) & ~(HASH_STR_NBLW_MASK);
// 		t |= (mbytes * 8) % 32;
// 		*(common.base + hash_str) = t;
// 		hal_cpuDataMemoryBarrier();
// 	}

// 	*(common.base + hash_str) |= HASH_STR_DCAL;
// 	hal_cpuDataMemoryBarrier();

// 	while ((*(common.base + hash_sr) & HASH_SR_DCIS) == 0) {
// 		; /* Wait for digest calculation to complete */
// 	}
// }


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


void hash_init(void)
{
	common.base = HASH_BASE;
	common.currBlocksize = 0;

	/* BOOT ROM can use this peripheral, so it's better to reset it. */
	_stm32_rccSetDevClock(dev_hash, 0);
	_stm32_rccDevReset(dev_hash, 1);
	_stm32_rccDevReset(dev_hash, 0);
	_stm32_rccSetDevClock(dev_hash, 1);

	return;
}
