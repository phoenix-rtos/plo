/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv8 Cortex-M
 *
 * Copyright 2017, 2019, 2020, 2022 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski, Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

/* Copy of armv7 Cortex-M architecture code */

#include "cpu.h"

struct {
	volatile u32 *scb;
} cpu_common;


/* clang-format off */
enum { scb_cpuid = 0, scb_icsr, scb_vtor, scb_aircr, scb_scr, scb_ccr, scb_shp0, scb_shp1,
	scb_shp2, scb_shcsr, scb_cfsr, scb_hfsr, scb_dfsr, scb_mmfar, scb_bfar, scb_afsr, scb_pfr0,
	scb_pfr1, scb_dfr, scb_afr, scb_mmfr0, scb_mmfr1, scb_mmfr2, scb_mmf3, scb_isar0, scb_isar1,
	scb_isar2, scb_isar3, scb_isar4, /* reserved */ scb_clidr = 30, scb_ctr, scb_ccsidr, scb_csselr,
	scb_cpacr, /* 93 reserved */ scb_stir = 128, /* 15 reserved */ scb_mvfr0 = 144, scb_mvfr1,
	scb_mvfr2, /* reserved */ scb_iciallu = 148, /* reserved */ scb_icimvau = 150, scb_dcimvac,
	scb_dcisw, scb_dccmvau, scb_dccmvac, scb_dccsw, scb_dccimvac, scb_dccisw, /* 6 reserved */
	scb_itcmcr = 164, scb_dtcmcr, scb_ahbpcr, scb_cacr, scb_ahbscr, /* reserved */ scb_abfsr = 170 };
/* clang-format on */


void hal_scbSetPriorityGrouping(u32 group)
{
	u32 t;

	/* Get register value and clear bits to set */
	t = *(cpu_common.scb + scb_aircr) & ~0xffff0700u;
	hal_cpuDataMemoryBarrier();

	/* Store new value */
	*(cpu_common.scb + scb_aircr) = t | 0x5fa0000u | ((group & 7u) << 8);
	hal_cpuDataMemoryBarrier();
}


u32 hal_scbGetPriorityGrouping(void)
{
	return (*(cpu_common.scb + scb_aircr) & 0x700u) >> 8;
}


void hal_scbSetPriority(s8 excpn, u32 priority)
{
	volatile u8 *ptr;

	ptr = &((u8 *)(cpu_common.scb + scb_shp0))[excpn - 4];

	*ptr = (priority << 4) & 0x0ffu;
	hal_cpuDataMemoryBarrier();
}


u32 hal_scbGetPriority(s8 excpn)
{
	volatile u8 *ptr;

	ptr = &((u8 *)(cpu_common.scb + scb_shp0))[excpn - 4];

	return *ptr >> 4;
}


/* Cache management */


/* Get CCSIDR value of L1 data cache */
static u32 hal_cache_getCCSIDR(void)
{
	*(cpu_common.scb + scb_csselr) = 0;
	hal_cpuInstrBarrier();
	return *(cpu_common.scb + scb_ccsidr);
}


static inline u32 hal_cache_getSets(u32 ccsidr)
{
	return ((ccsidr >> 13) & 0x7fffu) + 1;
}


static inline u32 hal_cache_getWays(u32 ccsidr)
{
	return ((ccsidr >> 3) & 0x3ffu) + 1;
}


/* Get Log2 of line size in bytes */
static inline u32 hal_cache_getLogLine(u32 ccsidr)
{
	return (ccsidr & 0x7) + 4;
}


/* Loop over sets and ways */
static void hal_cache_loopSetWay(u32 ccsidr, u32 reg_offset)
{
	u32 sets = hal_cache_getSets(ccsidr);
	u32 ways = hal_cache_getWays(ccsidr);
	hal_cpuDataSyncBarrier();
	for (u32 set = 0; set < sets; set++) {
		for (u32 way = 0; way < ways; way++) {
			*(cpu_common.scb + reg_offset) = ((set & 0x1ffu) << 5) | ((way & 0x3u) << 30);
		}
	}
	hal_cpuDataSyncBarrier();
}


/* Loop over memory locations */
static void hal_cache_loopMemory(u32 ccsidr, u32 reg_offset, addr_t start, size_t size)
{
	u32 lineSize = 1UL << hal_cache_getLogLine(ccsidr);
	size += start & (lineSize - 1);
	start &= ~(lineSize - 1);
	hal_cpuDataSyncBarrier();
	while (size >= lineSize) {
		*(cpu_common.scb + reg_offset) = start;
		start += lineSize;
		size -= lineSize;
	}

	if (size > 0) {
		/* If size is not aligned to line end, perform the operation on one additional line */
		*(cpu_common.scb + reg_offset) = start;
	}

	hal_cpuDataSyncBarrier();
}


void hal_enableDCache(void)
{
	if ((*(cpu_common.scb + scb_ccr) & (1 << 16)) == 0) {
		hal_cache_loopSetWay(hal_cache_getCCSIDR(), scb_dcisw);
		*(cpu_common.scb + scb_ccr) |= 1u << 16;

		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}
}


void hal_disableDCache(void)
{
	if ((*(cpu_common.scb + scb_ccr) & (1u << 16)) != 0) {
		*(cpu_common.scb + scb_ccr) &= ~(1u << 16);
		hal_cache_loopSetWay(hal_cache_getCCSIDR(), scb_dcisw);
		hal_cpuInstrBarrier();
	}
}


void hal_cleanInvalDCacheAddr(void *addr, u32 sz)
{
	hal_cache_loopMemory(hal_cache_getCCSIDR(), scb_dccimvac, (addr_t)addr, sz);
	hal_cpuInstrBarrier();
}


void hal_cleanDCache(void)
{
	hal_cache_loopSetWay(hal_cache_getCCSIDR(), scb_dccsw);
	hal_cpuInstrBarrier();
}


void hal_invalDCacheAddr(void *addr, u32 sz)
{
	hal_cache_loopMemory(hal_cache_getCCSIDR(), scb_dcimvac, (addr_t)addr, sz);
	hal_cpuInstrBarrier();
}


void hal_invalDCacheAll(void)
{
	hal_cache_loopSetWay(hal_cache_getCCSIDR(), scb_dcisw);
	hal_cpuInstrBarrier();
}


void hal_enableICache(void)
{
	if (!(*(cpu_common.scb + scb_ccr) & (1u << 17))) {
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
		*(cpu_common.scb + scb_iciallu) = 0u; /* Invalidate I$ */
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
		*(cpu_common.scb + scb_ccr) |= 1u << 17;
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}
}


void hal_disableICache(void)
{
	if (*(cpu_common.scb + scb_ccr) & (1u << 17)) {
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
		*(cpu_common.scb + scb_ccr) &= ~(1u << 17);
		*(cpu_common.scb + scb_iciallu) = 0u;
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}
}


void hal_invalICache(void)
{
	/* Check if ICache is enabled */
	if ((*(cpu_common.scb + scb_ccr) & (1u << 17)) != 0) {
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
		*(cpu_common.scb + scb_iciallu) = 0u;
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}
}


void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz)
{
	switch (type) {
		case hal_cpuDCache:
			if (sz == (size_t)-1) {
				hal_invalDCacheAll();
			}
			else {
				hal_invalDCacheAddr((void *)addr, sz);
			}
			break;

		case hal_cpuICache:
			/* TODO */
		default:
			break;
	}
}


unsigned int hal_cpuID(void)
{
	return *(cpu_common.scb + scb_cpuid);
}


void hal_cpuReboot(void)
{
	hal_cpuDataSyncBarrier();
	*(cpu_common.scb + scb_aircr) = ((0x5fau << 16) | (*(cpu_common.scb + scb_aircr) & (0x700u)) | (1 << 0x02));
	hal_cpuDataMemoryBarrier();

	for (;;) {
		;
	}
}


void hal_cpuInit(void)
{
	cpu_common.scb = (void *)0xe000ed00u;

	hal_scbSetPriorityGrouping(3);

	/* Configure cache */
	hal_enableDCache();
	hal_enableICache();

	/* Enable UsageFault, BusFault and MemManage exceptions */
	*(cpu_common.scb + scb_shcsr) |= (1u << 16) | (1u << 17) | (1u << 18);
	hal_cpuDataMemoryBarrier();

	/* Disable deep sleep */
	*(cpu_common.scb + scb_scr) &= ~(1u << 2);
	hal_cpuDataSyncBarrier();
}
