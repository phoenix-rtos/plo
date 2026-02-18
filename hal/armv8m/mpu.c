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

#define MPU_BASE ((void *)0xe000ed90)

#define MPU_MAX_REGIONS 16


struct {
	u32 type;
	u32 regMax;
	addr_t kernelEntryPoint;
} mpu_common;


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
static int mpu_regionSet(hal_syspage_prog_t *progHal, unsigned int *idx, addr_t start, addr_t end, u32 rbarAttr, u32 rlarAttr, u32 mapId)
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

	progHal->mpu.table[*idx].rbar = (start & ~0x1f) | rbarAttr;
	progHal->mpu.table[*idx].rlar = (limit & ~0x1f) | rlarAttr;
	progHal->mpu.map[*idx] = mapId;

	*idx += 1;
	return EOK;
}


/* Invalidate range of regions */
static void mpu_regionInvalidate(hal_syspage_prog_t *progHal, u8 first, u8 last)
{
	unsigned int i;

	for (i = first; (i < last) && (i < mpu_common.regMax); i++) {
		/* set multi-map to none */
		progHal->mpu.map[i] = (u32)-1;

		/* mark i-th region as disabled */
		progHal->mpu.table[i].rlar = 0;

		/* set exec never */
		progHal->mpu.table[i].rbar = 1;
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
	mpu_common.kernelEntryPoint = (addr_t)-1;
}

void mpu_kernelEntryPoint(addr_t addr)
{
	mpu_common.kernelEntryPoint = addr;
}


static int mpu_isMapAlloced(hal_syspage_prog_t *progHal, u32 mapId)
{
	unsigned int i;

	for (i = 0; i < progHal->mpu.allocCnt; i++) {
		if (progHal->mpu.map[i] == mapId) {
			return 1;
		}
	}

	return 0;
}


static int mpu_regionAlloc(hal_syspage_prog_t *progHal, addr_t addr, addr_t end, u32 attr, u32 mapId, unsigned int enable)
{
	int res;
	unsigned int regCur = progHal->mpu.allocCnt;
	u32 rbarAttr, rlarAttr;

	if (mpu_common.regMax == 0) {
		/* regMax == 0 indicates no MPU support - return without error, otherwise targets
		 * without MPU wouldn't work at all. */
		return EOK;
	}

	rbarAttr = mpu_regionAttrsRbar(attr);
	rlarAttr = mpu_regionAttrsRlar(attr, enable);

	res = mpu_regionSet(progHal, &regCur, addr, end, rbarAttr, rlarAttr, mapId);
	if (res != EOK) {
		mpu_regionInvalidate(progHal, progHal->mpu.allocCnt, regCur);
		return res;
	}

	progHal->mpu.allocCnt = regCur;

	return EOK;
}


static int mpu_allocKernelMap(hal_syspage_prog_t *progHal)
{
	addr_t start, end;
	u32 attr;
	u8 id;
	int res;
	const char *kcodemap = NULL;

	start = mpu_common.kernelEntryPoint;
	if (start == (addr_t)-1) {
		log_error("\nMissing kernel entry point!");
		return -EINVAL;
	}
	if ((res = syspage_mapAddrResolve(start, &kcodemap)) < 0) {
		return res;
	}
	if ((res = syspage_mapNameResolve(kcodemap, &id)) < 0) {
		return res;
	}
	if ((res = syspage_mapRangeResolve(kcodemap, &start, &end)) < 0) {
		return res;
	}
	if ((res = syspage_mapAttrResolve(kcodemap, &attr)) < 0) {
		return res;
	}
	if ((res = mpu_regionAlloc(progHal, start, end, attr, id, 1)) < 0) {
		log_error("\nCan't allocate MPU region for kernel code map (%s)", kcodemap);
		return res;
	}
	return EOK;
}


static void mpu_initPart(hal_syspage_prog_t *progHal)
{
	hal_memset(progHal, 0, sizeof(hal_syspage_prog_t));
	progHal->mpu.allocCnt = 0;
}


void mpu_getHalData(hal_syspage_t *hal)
{
	hal->mpuType = mpu_common.type;
}


static int mpu_mapsAlloc(hal_syspage_prog_t *progHal, const char *maps, size_t cnt)
{
	int i, res;
	addr_t start, end;
	u32 attr;
	u8 id;

	for (i = 0; i < cnt; i++) {
		if ((res = syspage_mapNameResolve(maps, &id)) < 0) {
			return res;
		}
		if (mpu_isMapAlloced(progHal, id) != 0) {
			maps += hal_strlen(maps) + 1; /* name + '\0' */
			continue;
		}
		if ((res = syspage_mapRangeResolve(maps, &start, &end)) < 0) {
			return res;
		}
		if ((res = syspage_mapAttrResolve(maps, &attr)) < 0) {
			return res;
		}
		if ((res = mpu_regionAlloc(progHal, start, end, attr, id, 1)) < 0) {
			log_error("\nCan't allocate MPU region for %s", maps);
			return res;
		}
		maps += hal_strlen(maps) + 1; /* name + '\0' */
	}
	return EOK;
}


extern int mpu_getHalProgData(syspage_prog_t *prog, const char *imaps, size_t imapSz, const char *dmaps, size_t dmapSz)
{
	int ret;

	mpu_initPart(&prog->hal);

	/* FIXME HACK
	 * allow all programs to execute (and read) kernel code map.
	 * Needed because of hal_jmp, syscalls handler and signals handler.
	 * In these functions we need to switch to the user mode when still
	 * executing kernel code. This will cause memory management fault
	 * if the application does not have access to the kernel instruction
	 * map. Possible fix - place return to the user code in the separate
	 * region and allow this region instead. */
	ret = mpu_allocKernelMap(&prog->hal);
	if (ret != EOK) {
		return ret;
	}
	ret = mpu_mapsAlloc(&prog->hal, imaps, imapSz);
	if (ret != EOK) {
		return ret;
	}
	ret = mpu_mapsAlloc(&prog->hal, dmaps, dmapSz);
	if (ret != EOK) {
		return ret;
	}

	mpu_regionInvalidate(&prog->hal, prog->hal.mpu.allocCnt, mpu_common.regMax);

	return EOK;
}
