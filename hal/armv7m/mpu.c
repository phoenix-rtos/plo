/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * MPU region allocation routines
 *
 * Copyright 2021 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/errno.h>
#include "mpu.h"


static mpu_common_t mpu_common;


enum { mpu_type, mpu_ctrl, mpu_rnr, mpu_rbar, mpu_rasr, mpu_rbar_a1, mpu_rasr_a1, mpu_rbar_a2, mpu_rasr_a2,
	   mpu_rbar_a3, mpu_rasr_a3 };


/* Binary count trailing zeros */
__attribute__((always_inline)) static inline u8 _ctz(u32 val)
{
	asm volatile(" \
		rbit %0, %0; \
		clz %0, %0"
		: "+l"(val));
	return val;
}


/* Setup single MPU region entry in a local MPU context */
static int mpu_regionSet(u8 idx, addr_t addr, u8 sizeBit, u32 attr, unsigned int enable, u8 srdMask)
{
	u8 tex = 0;
	u8 ap = 1; /* privileged read-write access, unprivileged no access */

	if (sizeBit < 5 || idx >= mpu_common.regMax)
		return -EPERM;

	if (attr & mAttrRead)
		ap = 2; /* privileged read-write access, unprivileged read only access */

	if (attr & mAttrWrite)
		ap = 3; /* privileged read-write access, unprivileged read and write access */

	mpu_common.region[idx].rbar =
		((u32)addr & (0x7fffffful << 5)) |
		(1ul << 4) | /* mark region as valid */
		(idx & 0xful);

	mpu_common.region[idx].rasr =
		((((attr & mAttrExec) == 0) & 1ul) << 28) |
		((ap & 0x7ul) << 24) |
		((tex & 0x7ul) << 19) |
		((((attr & mAttrShareable) != 0) & 1ul) << 18) |
		((((attr & mAttrCacheable) != 0) & 1ul) << 17) |
		((((attr & mAttrBufferable) != 0) & 1ul) << 16) |
		((srdMask & 0xfful) << 8) |
		(((sizeBit - 1) & 0x1ful) << 1) |
		(enable != 0);

	return EOK;
}


/* Find best alignment for map and setup a region */
static int mpu_regionBestFit(u8 idx, addr_t addr, size_t size, u32 attr, unsigned int enable, size_t *allocSize)
{
	size_t srSize;
	addr_t srBase, alignMask;

	u8 alignBits = _ctz(addr);
	u8 srdMask = 0, bit = _ctz(size);

	if (bit < alignBits)
		alignBits = bit;

	alignMask = alignBits < 32 ? ~((1u << alignBits) - 1) : 0;

	if (alignBits < 5) {
		*allocSize = 1u << alignBits;
	}
	else {
		alignBits += 3;

		if (alignBits >= 32) {
			alignBits = 32;
			alignMask = 0;
		}
		else {
			alignMask = ~((1u << alignBits) - 1);
		}

		*allocSize = 0;
		srBase = addr & alignMask;
		srSize = 1u << (alignBits - 3);

		for (bit = 0; bit < 8; bit++) {
			if (srBase < addr || srBase + srSize > addr + size || *allocSize + srSize > size)
				srdMask |= 1u << bit;
			else
				*allocSize += srSize;

			srBase += srSize;
		}
	}

	return mpu_regionSet(idx, addr & alignMask, alignBits, attr, enable, srdMask);
}


/* Invalidate range of regions */
static void mpu_regionInvalidate(u8 first, u8 last)
{
	unsigned int i;

	for (i = first; i < last && i < mpu_common.regMax; i++) {
		/* set multi-map to none */
		mpu_common.mapId[i] = (u32)-1;

		/* mark i-th region as invalid */
		mpu_common.region[i].rbar = i & 0xful;

		/* set exec never and mark whole region as disabled */
		mpu_common.region[i].rasr = (1ul << 28) | (0x1ful << 1);
	}
}


/* Assign range of regions a multi-map id */
static void mpu_regionAssignMap(u8 first, u8 last, u32 mapId)
{
	unsigned int i;

	for (i = first; i < last && i < mpu_common.regMax; i++) {
		/* assign map only if region is valid */
		if (mpu_common.region[i].rbar & (1ul << 4))
			mpu_common.mapId[i] = mapId;
	}
}


const mpu_common_t *const mpu_getCommon(void)
{
	return &mpu_common;
}


void mpu_init(void)
{
	volatile u32 *mpu_base = (void *)0xe000ed90;

	mpu_common.type = *(mpu_base + mpu_type);
	mpu_common.regMax = (u8)(mpu_common.type >> 8);
	mpu_common.regCnt = mpu_common.mapCnt = 0;

	mpu_regionInvalidate(0, sizeof(mpu_common.region) / sizeof(mpu_common.region[0]));
}


int mpu_regionAlloc(addr_t addr, addr_t end, u32 attr, u32 mapId, unsigned int enable)
{
	int res = EOK;
	size_t size, allocSize = 0;
	u8 regCur = mpu_common.regCnt;

	if (addr > end)
		return -ERANGE;

	size = end - addr;

	while (size > 0 && (regCur - mpu_common.regCnt) < 2) {
		if ((res = mpu_regionBestFit(regCur++, addr, size, attr, enable, &allocSize)) < 0)
			break;

		if (allocSize > size)
			break;

		addr += allocSize;
		size -= allocSize;
	}

	if (res != EOK || size != 0) {
		mpu_regionInvalidate(mpu_common.regCnt, regCur);
		return res == EOK ? -EPERM : res;
	}

	mpu_regionAssignMap(mpu_common.regCnt, regCur, mapId);

	mpu_common.regCnt = regCur;
	mpu_common.mapCnt++;

	return EOK;
}


void mpu_getHalData(hal_syspage_t *hal)
{
	unsigned int i;

	mpu_regionInvalidate(mpu_common.regCnt, mpu_common.regMax);

	hal->mpu.type = mpu_common.type;
	hal->mpu.allocCnt = mpu_common.regCnt;

	for (i = 0; i < sizeof(hal->mpu.table) / sizeof(hal->mpu.table[0]); i++) {
		hal->mpu.table[i].rbar = mpu_common.region[i].rbar;
		hal->mpu.table[i].rasr = mpu_common.region[i].rasr;
		hal->mpu.map[i] = mpu_common.mapId[i];
	}
}
