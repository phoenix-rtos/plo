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


static void hash_process(const u8 *mess, u32 mbytes, u32 blocksize)
{
	u32 off = 0;
	u32 written = 0;
	u32 t, toff;

	while (mbytes >= 4) {
		*(common.base + hash_din) = *((const u32 *)(mess + off));
		hal_cpuDataMemoryBarrier();
		off += 4;
		written += 4;
		mbytes -= 4;

		if (written > blocksize) {
			while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
				; /* Wait for partial digest computation */
			}
			written -= blocksize;
		}
	}
	if (mbytes > 0) {
		toff = 0;
		t = 0;
		while (toff < mbytes) {
			t |= (*(mess + off) << (toff * 8));
			toff += 1;
			off += 1;
		}
		*(common.base + hash_din) = t;
		hal_cpuDataMemoryBarrier();
		written += mbytes;

		if (written > blocksize) {
			while ((*(common.base + hash_sr) & HASH_SR_DINIS) == 0) {
				; /* Wait for partial digest computation */
			}
			written -= blocksize;
		}

		t = *(common.base + hash_str) & ~(HASH_STR_NBLW_MASK);
		t |= (mbytes * 8) % 32;
		*(common.base + hash_str) = t;
		hal_cpuDataMemoryBarrier();
	}

	*(common.base + hash_str) |= HASH_STR_DCAL;
	hal_cpuDataMemoryBarrier();

	while ((*(common.base + hash_sr) & HASH_SR_DCIS) == 0) {
		; /* Wait for digest calculation to complete */
	}
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


/* Calculate hash digest. Message length counted in bytes. Output digest in little endian.
 * HASH peripheral supports message sizes measured in bits, but we don't need that kind of granularity.
 * User has to provide bith algorithm type and the number of bytes of the output hash. */
int hash_digest(u32 algo, const u8 *mess, u32 mbytes, u8 *out, u32 outbytes)
{
	/* Assume correct arguments */
	u32 blocksize = 0;

	/* Deduce message block size */
	blocksize = hash_getBlockSize(algo);

	lib_printf("\nblocksize: %d\n", blocksize);

	/* HASH accepts data in big endian. Enable per byte data swapping */
	*(common.base + hash_cr) = algo | HASH_CR_SWAP_BYTES;
	/* Set number of valid bits in the last 32-bit message word. 0 if all valid. */
	*(common.base + hash_str) = (*(common.base + hash_str) & ~(HASH_STR_NBLW_MASK)) | ((mbytes * 8) % 32);
	hal_cpuDataMemoryBarrier();

	*(common.base + hash_cr) |= HASH_CR_INIT;
	hal_cpuDataMemoryBarrier();

	hash_process(mess, mbytes, blocksize);
	hash_copyDigest(out, outbytes);

	return EOK;
}

void hash_init(void)
{
	common.base = HASH_BASE;

	/* BOOT ROM can use this peripheral, so it's better to reset it. */
	_stm32_rccSetDevClock(dev_hash, 0);
	_stm32_rccDevReset(dev_hash, 1);
	_stm32_rccDevReset(dev_hash, 0);
	_stm32_rccSetDevClock(dev_hash, 1);

	return;
}
