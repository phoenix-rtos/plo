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
#include "lib/log.h"
#include "syspage.h"


static mpu_common_t mpu_common;


/* clang-format off */
enum { mpu_type, mpu_ctrl, mpu_rnr, mpu_rbar, mpu_rasr, mpu_rbar_a1, mpu_rasr_a1, mpu_rbar_a2, mpu_rasr_a2,
	   mpu_rbar_a3, mpu_rasr_a3 };
/* clang-format on */


/* Removes all RASR attribute bits except ENABLE */
#define HOLE_ATTR(rasrAttr) (0 | ((rasrAttr) & 0x1))


/* Setup single MPU region entry in a local MPU context */
static int mpu_regionSet(unsigned int *idx, u32 baseAddr, u8 srdMask, u8 sizeBit, u32 rasrAttr)
{
	mpu_part_t *mpu = &mpu_common.curPart;

	if ((sizeBit < 5) || (*idx >= mpu_common.regMax)) {
		return -EPERM;
	}

	mpu->region[*idx].rbar =
			baseAddr |
			(1u << 4) | /* mark region as valid */
			(*idx & 0xfu);

	mpu->region[*idx].rasr =
			rasrAttr |
			(((u32)srdMask) << 8) |
			((((u32)sizeBit - 1) & 0x1fu) << 1);

	*idx += 1;
	return EOK;
}


/* Translate memory map attributes to RASR attribute bits */
static u32 mpu_regionAttrs(u32 attr, unsigned int enable)
{
	u8 tex = 0;
	u8 ap = 1; /* privileged read-write access, unprivileged no access */

	if ((attr & mAttrRead) != 0) {
		ap = 2; /* privileged read-write access, unprivileged read only access */
	}

	if ((attr & mAttrWrite) != 0) {
		ap = 3; /* privileged read-write access, unprivileged read and write access */
	}

	return ((((attr & mAttrExec) == 0) & 1u) << 28) |
			((ap & 0x7u) << 24) |
			((tex & 0x7u) << 19) |
			((((attr & mAttrShareable) != 0) & 1u) << 18) |
			((((attr & mAttrCacheable) != 0) & 1u) << 17) |
			((((attr & mAttrBufferable) != 0) & 1u) << 16) |
			(enable != 0);
}


static int mpu_checkOverlap(unsigned int idx, u32 start, u32 end)
{
	mpu_part_t *mpu = &mpu_common.curPart;

	unsigned int i, j;
	u32 srStart, srEnd;
	u8 sizeBit, subregions;
	end -= 1;
	for (i = 0; i < idx; i++) {
		if (((mpu->region[i].rbar & 0x10u) == 0) || ((mpu->region[i].rasr & 0x1u) == 0)) {
			continue;
		}

		sizeBit = ((mpu->region[i].rasr >> 1) & 0x1fu) + 1;
		srStart = mpu->region[i].rbar & ~((1u << sizeBit) - 1);
		subregions = (mpu->region[i].rasr >> 8) & 0xffu;
		for (j = 0; j < 8; j++) {
			srEnd = srStart + (1u << (sizeBit - 3)) - 1;
			if (((subregions & 1u) == 0) && (start <= srEnd) && (srStart <= end)) {
				return 1;
			}

			srStart = srEnd + 1;
			subregions >>= 1;
		}
	}

	return 0;
}


static int mpu_regionCalculateAndSet(unsigned int *idx, addr_t start, addr_t end, u8 sizeBit, u32 rasrAttr)
{
	u8 srdMask;
	/* RBAR contains all MSBs that are the same */
	u32 baseAddr = start & ~((1u << sizeBit) - 1);
	/* Extract first enabled and first disabled region */
	u8 srStart = (start >> (sizeBit - 3)) & 7u;
	u8 srEnd = (end >> (sizeBit - 3)) & 7u;
	srEnd = (srEnd == 0) ? 8 : srEnd;
	/* Bit set means disable region - negate result */
	srdMask = ~(((1u << srEnd) - 1) & (0xffu << srStart));
	return mpu_regionSet(idx, baseAddr, srdMask, sizeBit, rasrAttr);
}


/* Create up to 2 regions that will represent a given map */
static int mpu_regionGenerate(unsigned int *idx, addr_t start, addr_t end, u32 rasrAttr)
{
	int res;
	int commonTrailingZeroes, sigBits;
	u32 diffMask, alignedStart, alignedEnd, holeStart, holeEnd;
	u8 commonMsb, sizeBit;
	addr_t reg1End;
	const size_t size = (end - start) & 0xffffffffu;

	/* Allow end == 0, this means end of address range */
	if ((end != 0) && (end <= start)) {
		return -EINVAL;
	}

	/* Check if size is power of 2 and start is aligned - necessary for handling
	   small regions (below 256 bytes) */
	if (size == 0) {
		return mpu_regionSet(idx, 0, 0, 32, rasrAttr);
	}

	if ((size == (1u << __builtin_ctz(size))) && ((start & (size - 1)) == 0)) {
		if (size < 32) {
			/* Not supported by MPU */
			return -EPERM;
		}

		return mpu_regionSet(idx, start, 0, __builtin_ctz(size), rasrAttr);
	}

	commonTrailingZeroes = __builtin_ctz(start | end);
	if (commonTrailingZeroes < 5) {
		/* This would require subregions smaller than 32 bytes to represent - not supported by MPU */
		return -EPERM;
	}

	commonMsb = 32 - __builtin_clz(start ^ ((end - 1) & 0xffffffffu));
	sigBits = commonMsb - commonTrailingZeroes;
	if (sigBits <= 3) {
		/* Can be represented with one region + 8 subregions */
		sizeBit = commonTrailingZeroes + 3;
		return mpu_regionCalculateAndSet(idx, start, end, sizeBit, rasrAttr);
	}
	else if (sigBits == 4) {
		/* Can be represented with 2 regions + up to 8 subregions each */
		sizeBit = commonTrailingZeroes + 3;
		diffMask = (1u << sizeBit) - 1;
		reg1End = (start & (~diffMask)) + diffMask + 1;
		res = mpu_regionCalculateAndSet(idx, start, reg1End, sizeBit, rasrAttr);
		return (res == EOK) ? mpu_regionCalculateAndSet(idx, reg1End, end, sizeBit, rasrAttr) : res;
	}
	else if (rasrAttr == HOLE_ATTR(rasrAttr)) {
		/* Cannot attempt another cutout - we are already trying to make a hole */
		return -EPERM;
	}

	/* Attempt to allocate larger region and mask start or end with another region */
	diffMask = (1u << (commonMsb - 3)) - 1;
	if ((start & (~diffMask)) == start) {
		/* Start aligned - try cutting from the end */
		alignedStart = start;
		alignedEnd = (end & (~diffMask)) + diffMask + 1;
		holeStart = end;
		holeEnd = alignedEnd;
	}
	else if ((end & (~diffMask)) == end) {
		/* End aligned - try cutting from the start */
		alignedStart = start & (~diffMask);
		alignedEnd = end;
		holeStart = alignedStart;
		holeEnd = start;
	}
	else {
		/* Would need cutting from both ends - not supported (we limit to 2 regions per map) */
		return -EPERM;
	}

	/* First check if our "hole" overrides any existing mappings. This would lead to unintuitive behaviors. */
	if (mpu_checkOverlap(*idx, holeStart, holeEnd) != 0) {
		return -EPERM;
	}

	res = mpu_regionCalculateAndSet(idx, alignedStart, alignedEnd, commonMsb, rasrAttr);
	return (res == EOK) ? mpu_regionGenerate(idx, holeStart, holeEnd, HOLE_ATTR(rasrAttr)) : res;
}


/* Invalidate range of regions */
static void mpu_regionInvalidate(u8 first, u8 last)
{
	unsigned int i;
	mpu_part_t *mpu = &mpu_common.curPart;

	for (i = first; i < last && i < mpu_common.regMax; i++) {
		/* set multi-map to none */
		mpu->mapId[i] = (u32)-1;

		/* mark i-th region as invalid */
		mpu->region[i].rbar = i & 0xfu;

		/* set exec never and mark whole region as disabled */
		mpu->region[i].rasr = (1u << 28) | (0x1fu << 1);
	}
}


/* Assign range of regions a multi-map id */
static void mpu_regionAssignMap(u8 first, u8 last, u32 mapId)
{
	unsigned int i;
	mpu_part_t *mpu = &mpu_common.curPart;


	for (i = first; i < last && i < mpu_common.regMax; i++) {
		/* assign map only if region is valid */
		if (mpu->region[i].rbar & (1u << 4))
			mpu->mapId[i] = mapId;
	}
}


static int mpu_isMapAlloced(u32 mapId)
{
	unsigned int i;
	mpu_part_t *mpu = &mpu_common.curPart;

	for (i = 0; i < mpu->regCnt; i++) {
		if (mpu->mapId[i] == mapId) {
			return 1;
		}
	}

	return 0;
}


static void mpu_initPart(void)
{
	mpu_part_t *mpu = &mpu_common.curPart;

	hal_memset(mpu, 0, sizeof(mpu_part_t));
	mpu->regCnt = 0;
	mpu_regionInvalidate(0, sizeof(mpu->region) / sizeof(mpu->region[0]));
}


static int mpu_regionAlloc(addr_t addr, addr_t end, u32 attr, u32 mapId, unsigned int enable)
{
	int res;
	mpu_part_t *mpu = &mpu_common.curPart;
	unsigned int regCur = mpu->regCnt;
	u32 rasrAttr = mpu_regionAttrs(attr, enable);

	int regbeg = regCur;
	res = mpu_regionGenerate(&regCur, addr, end, rasrAttr);
	if (res != EOK) {
		mpu_regionInvalidate(mpu->regCnt, regCur);
		return res;
	}
	hal_consolePrint("REGIONS: ");
	char *c = "0";
	c[0] = '0' + (regCur - regbeg);
	hal_consolePrint(c);
	hal_consolePrint(" regions used\n");

	mpu_regionAssignMap(mpu->regCnt, regCur, mapId);

	mpu->regCnt = regCur;

	return EOK;
}


const mpu_common_t *const mpu_getCommon(void)
{
	return &mpu_common;
}


void mpu_init(void)
{
	u32 mpu_type;

	__asm__ volatile("mrc p15, 0, %0, c0, c0, 4" : "=r"(mpu_type));
	mpu_common.type = mpu_type;
	mpu_common.regMax = (u8)(mpu_common.type >> 8);
}


void mpu_getHalData(hal_syspage_t *hal)
{
	hal->mpu.type = mpu_common.type;
}


static void mpu_mapsAlloc(const char *maps, size_t cnt)
{
	int i, res;
	addr_t start, end;
	u32 attr;
	u8 id;

	for (i = 0; i < cnt; i++) {
		// TODO: merge regions with same attributes and adjacent addresses
		// TODO: more sophisticated merging? (hole punching, common subregions etc.)
		if ((res = syspage_mapNameResolve(maps, &id)) < 0) {
			log_error("\nCan't add map %s", maps);
			// return res;
		}
		if (mpu_isMapAlloced(id) != 0) {
			maps += hal_strlen(maps) + 1; /* name + '\0' */
			continue;
		}
		if ((res = syspage_mapRangeResolve(maps, &start, &end)) < 0) {
			log_error("\nCan't resolve range for %s", maps);
			// return res;
		}
		if ((res = syspage_mapAttrResolve(maps, &attr)) < 0) {
			log_error("\nCan't resolve attributes for %s", maps);
			// return res;
		}
		log_error("\nAllocating map %s: [%08x-%08x) attr=%x id=%d", maps, start, end, attr, id);
		maps += hal_strlen(maps) + 1; /* name + '\0' */

		mpu_regionAlloc(start, end, attr, id, 1);
	}
}


extern void mpu_getProgHal(hal_syspage_prog_t *progHal, const char *imaps, size_t imapSz, const char *dmaps, size_t dmapSz)
{
	unsigned int i;
	mpu_part_t *mpu = &mpu_common.curPart;

	mpu_initPart();

	mpu_mapsAlloc("kddr", 1);
	mpu_mapsAlloc(imaps, imapSz);
	mpu_mapsAlloc(dmaps, dmapSz);

	mpu_regionInvalidate(mpu->regCnt, mpu_common.regMax);

	progHal->allocCnt = mpu->regCnt;

	for (i = 0; i < sizeof(progHal->table) / sizeof(progHal->table[0]); i++) {
		progHal->table[i].rbar = mpu->region[i].rbar;
		progHal->table[i].rasr = mpu->region[i].rasr;
		progHal->map[i] = mpu->mapId[i];  // TODO: is this still needed?
	}
}
