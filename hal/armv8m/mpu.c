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

#ifndef DISABLE_MPU
#define DISABLE_MPU 0
#endif

static mpu_common_t mpu_common;


enum {
	mpu_type,
	mpu_ctrl,
	mpu_rnr,
	mpu_rbar,
	mpu_rlar,
	mpu_rbar_a1,
	mpu_rlar_a1,
	mpu_rbar_a2,
	mpu_rlar_a2,
	mpu_rbar_a3,
	mpu_rlar_a3,
	mpu_mair0 = 0xC,
	mpu_mair1
};


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

	mpu_common.region[*idx].rbar = (start & ~0x1f) | rbarAttr;
	mpu_common.region[*idx].rlar = (limit & ~0x1f) | rlarAttr;
	mpu_common.mapId[*idx] = mapId;

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

	for (i = first; (i < last) && (i < mpu_common.regMax); i++) {
		/* set multi-map to none */
		mpu_common.mapId[i] = (u32)-1;

		/* mark i-th region as disabled */
		mpu_common.region[i].rlar = 0;

		/* set exec never */
		mpu_common.region[i].rbar = 1;
	}
}


void mpu_init(void)
{
#if DISABLE_MPU
	mpu_common.type = 0;
	mpu_common.regMax = 0;
#else
	volatile u32 *mpu_base = MPU_BASE;
	/* MPU_TYPE register is always implemented on ARMv8-M. DREGION field indicates number of regions supported,
	 * 0 indicates that processor does not implement an MPU. */
	mpu_common.type = *(mpu_base + mpu_type);
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
		*(mpu_base + mpu_mair0) = 0xee04aa00;
	}
#endif
	mpu_common.regCnt = 0;
	mpu_common.mapCnt = 0;

	mpu_regionInvalidate(0, MPU_MAX_REGIONS);
}


int mpu_regionAlloc(addr_t addr, addr_t end, u32 attr, u32 mapId, unsigned int enable)
{
	int res;
	unsigned int regCur = mpu_common.regCnt;
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
		mpu_regionInvalidate(mpu_common.regCnt, regCur);
		return res;
	}

	mpu_common.regCnt = regCur;
	mpu_common.mapCnt++;

	return EOK;
}

void mpu_getHalData(hal_syspage_t *hal)
{
	const unsigned int syspageTableSize = sizeof(hal->mpu.table) / sizeof(hal->mpu.table[0]);
	unsigned int i;

	mpu_regionInvalidate(mpu_common.regCnt, mpu_common.regMax);

	if (mpu_common.regCnt > syspageTableSize) {
		/* TODO: We need a way to handle errors here that can happen due to misconfiguration
		 * (size of table in syspage too small). This way to do it silently truncates the list
		 * which may cause undesired behaviors. */
		mpu_common.regCnt = syspageTableSize;
	}

	hal->mpu.type = mpu_common.type;
	hal->mpu.allocCnt = mpu_common.regCnt;

	for (i = 0; i < mpu_common.regCnt; i++) {
		hal->mpu.table[i].rbar = mpu_common.region[i].rbar;
		hal->mpu.table[i].rlar = mpu_common.region[i].rlar;
		hal->mpu.map[i] = mpu_common.mapId[i];
	}

	/* Ensure all entries are initialized if mpu_common.regCnt or mpu_common.regMax is smaller than syspageTableSize */
	for (; i < syspageTableSize; i++) {
		hal->mpu.map[i] = (u32)-1;  /* set multi-map to none */
		hal->mpu.table[i].rlar = 0; /* mark i-th region as disabled */
		hal->mpu.table[i].rbar = 1; /* set exec never */
	}
}
