/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * MPU API
 *
 * Copyright 2021, 2022, 2025 Phoenix Systems
 * Author: Gerard Swiderski, Damian Loewnau, Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/errno.h>
#include <board_config.h>
#include "mpu.h"
#include "lib/log.h"
#include "syspage.h"

#ifndef DISABLE_MPU
#define DISABLE_MPU 0
#endif

static mpu_common_t mpu_common;


/* Translate memory map attributes to RLAR attribute bits */
static u32 mpu_regionAttrsRlar(u32 attr, unsigned int enable)
{
	u8 attrIndx = 0, execNever = 0;
	attrIndx |= ((attr & mAttrCacheable) != 0) ? 1 : 0;
	attrIndx |= ((attr & mAttrBufferable) != 0) ? (1 << 1) : 0;
	execNever |= ((attr & mAttrExec) == 0) ? 1 : 0;
	return (execNever << 4) | (attrIndx << 1) | ((enable != 0) ? 1 : 0);
}


/* Translate memory map attributes to RBAR attribute bits */
static u32 mpu_regionAttrsRbar(u32 attr)
{
	u8 ap = 0; /* set privileged read-write access, unprivileged no access */

	if ((attr & mAttrRead) != 0) {
		/*
		 * set privileged read-write access, unprivileged read only access
		 * TODO: Privileged read/write and unprivileged read/only configuration is not supported
		 * on ARMv8-M. As a workaround for now we treat it as read/write for both privilege levels.
		 */
		ap = 1;
	}

	if ((attr & mAttrWrite) != 0) {
		ap = 1; /* set privileged read-write access, unprivileged read and write access */
	}

	return ((((attr & mAttrShareable) != 0) ? 1 : 0) << 4) |
			(ap << 1) |
			(((attr & mAttrExec) == 0) ? 1 : 0);
}


/* Setup single MPU region entry in local MPU context */
static int mpu_regionSet(unsigned int *idx, addr_t start, addr_t end, u32 rbarAttr, u32 rlarAttr, u32 mapId)
{
	/* Allow end == 0, this means end of address range */
	const size_t size = (end - start) & 0xffffffffu;
	u32 limit = end - 1;
	mpu_part_t *mpu = &mpu_common.curPart;

	if (*idx >= mpu_common.regMax) {
		return -EPERM;
	}

	/* Allow end == 0, this means end of address range */
	if ((end != 0) && (end <= start)) {
		return -EINVAL;
	}

	/* Check if entire address range is requested */
	if (size == 0) {
		limit = 0xffffffff;
	}
	else if (size < 32 || ((size & 0x1f) != 0) || ((start & 0x1f) != 0)) {
		/* Not supported by MPU */
		return -EPERM;
	}

	mpu->region[*idx].rbar = (start & ~0x1f) | rbarAttr;
	mpu->region[*idx].rlar = (limit & ~0x1f) | rlarAttr;
	mpu->mapId[*idx] = mapId;

	*idx += 1;
	return EOK;
}


const mpu_common_t *const mpu_getCommon(void)
{
	return &mpu_common;
}


/* Invalidate range of regions */
static void mpu_regionInvalidate(u8 first, u8 last)
{
	unsigned int i;
	mpu_part_t *mpu = &mpu_common.curPart;

	for (i = first; (i < last) && (i < mpu_common.regMax); i++) {
		/* set multi-map to none */
		mpu->mapId[i] = (u32)-1;

		/* mark i-th region as disabled */
		mpu->region[i].rlar = 0;

		/* set exec never */
		mpu->region[i].rbar = 1;
	}
}


void mpu_init(void)
{
#if DISABLE_MPU
	mpu_common.type = 0;
	mpu_common.regMax = 0;
#else
	/* MPU_TYPE register is always implemented on ARMv8-M. DREGION field indicates number of regions supported,
	 * 0 indicates that processor does not implement an MPU. */
	asm volatile("MRC p15, 0, %0, c0, c0, 4" ::"r"(mpu_common.type));

	mpu_common.regMax = (mpu_common.type >> 8) & 0xff;
	/* Hardware may support more regions than can be stored in our code. */
	if (mpu_common.regMax > MPU_MAX_REGIONS) {
		mpu_common.regMax = MPU_MAX_REGIONS;
	}

	if (mpu_common.regMax != 0) {
		/*
		 * syspage structure lacks fields for MAIR registers, instead programming them in plo
		 * MPU_MAIR0 Attr<n>:
		 * 3: Cacheable & Bufferable -> outer and inner write back, read alloc policy
		 * 1: Cacheable & !Bufferable -> outer and inner Write-Through, read alloc policy
		 * 2: !Cacheable & Bufferable -> device memory (armv7m), nGnRE
		 * 0: !Cacheable & !Bufferable -> device memory (armv7m strongly ordered) nGnRnE
		 */
		// *(mpu_base + mpu_mair0) = 0xee04aa00; /* TODO - implement! */
	}
#endif
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
	u32 rbarAttr, rlarAttr;

	if (mpu_common.regMax == 0) {
		/* regMax == 0 indicates no MPU support - return without error, otherwise targets
		 * without MPU wouldn't work at all. */
		return EOK;
	}

	rbarAttr = mpu_regionAttrsRbar(attr);
	rlarAttr = mpu_regionAttrsRlar(attr, enable);

	res = mpu_regionSet(&regCur, addr, end, rbarAttr, rlarAttr, mapId);
	if (res != EOK) {
		mpu_regionInvalidate(mpu->regCnt, regCur);
		return res;
	}

	mpu->regCnt = regCur;

	return EOK;
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
		maps += hal_strlen(maps) + 1; /* name + '\0' */

		mpu_regionAlloc(start, end, attr, id, 1);
	}
}


extern void mpu_getProgHal(hal_syspage_prog_t *progHal, const char *imaps, size_t imapSz, const char *dmaps, size_t dmapSz)
{
	unsigned int i;
	mpu_part_t *mpu = &mpu_common.curPart;

	mpu_initPart();

	// mpu_mapsAlloc("itcm", 1);  // - TODO: where is kernel data???
	mpu_mapsAlloc(imaps, imapSz);
	mpu_mapsAlloc(dmaps, dmapSz);

	mpu_regionInvalidate(mpu->regCnt, mpu_common.regMax);

	progHal->allocCnt = mpu->regCnt;

	for (i = 0; i < sizeof(progHal->table) / sizeof(progHal->table[0]); i++) {
		progHal->table[i].rbar = mpu->region[i].rbar;
		progHal->table[i].rlar = mpu->region[i].rlar;
		progHal->map[i] = mpu->mapId[i];  // TODO: is this still needed?
	}
}
