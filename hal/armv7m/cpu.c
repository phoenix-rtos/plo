/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv7 Cortex-M
 *
 * Copyright 2017, 2019, 2020, 2023 Phoenix Systems
 * Author: Aleksander Kaminski, Jan Sikorski, Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cpu.h"

struct {
	volatile u32 *scb;
} cpu_common;


enum { scb_cpuid = 0, scb_icsr, scb_vtor, scb_aircr, scb_scr, scb_ccr, scb_shp0, scb_shp1,
	scb_shp2, scb_shcsr, scb_cfsr, scb_hfsr, scb_dfsr, scb_mmfar, scb_bfar, scb_afsr, scb_pfr0,
	scb_pfr1, scb_dfr, scb_afr, scb_mmfr0, scb_mmfr1, scb_mmfr2, scb_mmf3, scb_isar0, scb_isar1,
	scb_isar2, scb_isar3, scb_isar4, /* reserved */ scb_clidr = 30, scb_ctr, scb_ccsidr, scb_csselr,
	scb_cpacr, /* 93 reserved */ scb_stir = 128, /* 15 reserved */ scb_mvfr0 = 144, scb_mvfr1,
	scb_mvfr2, /* reserved */ scb_iciallu = 148, /* reserved */ scb_icimvau = 150, scb_dcimvac,
	scb_dcisw, scb_dccmvau, scb_dccmvac, scb_dccsw, scb_dccimvac, scb_dccisw, /* 6 reserved */
	scb_itcmcr = 164, scb_dtcmcr, scb_ahbpcr, scb_cacr, scb_ahbscr, /* reserved */ scb_abfsr = 170 };


void hal_scbSetPriorityGrouping(u32 group)
{
	u32 t;

	/* Get register value and clear bits to set */
	t = *(cpu_common.scb + scb_aircr) & ~0xffff0700;
	hal_cpuDataMemoryBarrier();

	/* Store new value */
	*(cpu_common.scb + scb_aircr) = t | 0x5fa0000 | ((group & 7) << 8);
	hal_cpuDataMemoryBarrier();
}


u32 hal_scbGetPriorityGrouping(void)
{
	return (*(cpu_common.scb + scb_aircr) & 0x700) >> 8;
}


void hal_scbSetPriority(s8 excpn, u32 priority)
{
	volatile u8 *ptr;

	ptr = &((u8 *)(cpu_common.scb + scb_shp0))[excpn - 4];

	*ptr = (priority << 4) & 0x0ff;
	hal_cpuDataMemoryBarrier();
}


u32 hal_scbGetPriority(s8 excpn)
{
	volatile u8 *ptr;

	ptr = &((u8 *)(cpu_common.scb + scb_shp0))[excpn - 4];

	return *ptr >> 4;
}


__attribute__((section(".noxip"))) void hal_enableDCache(void)
{
	u32 ccsidr, sets, ways;

	if (!(*(cpu_common.scb + scb_ccr) & (1 << 16))) {
		*(cpu_common.scb + scb_csselr) = 0;
		hal_cpuDataSyncBarrier();

		ccsidr = *(cpu_common.scb + scb_ccsidr);

		/* Invalidate D$ */
		sets = (ccsidr >> 13) & 0x7fff;
		do {
			ways = (ccsidr >> 3) & 0x3ff;
			do {
				*(cpu_common.scb + scb_dcisw) = ((sets & 0x1ff) << 5) | ((ways & 0x3) << 30);
			} while (ways-- != 0);
		} while (sets-- != 0);
		hal_cpuDataSyncBarrier();

		*(cpu_common.scb + scb_ccr) |= 1 << 16;

		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}
}


__attribute__((section(".noxip"))) void hal_disableDCache(void)
{
	register u32 ccsidr, sets, ways;

	if (*(cpu_common.scb + scb_ccr) & (1 << 16)) {
		*(cpu_common.scb + scb_csselr) = 0;
		hal_cpuDataSyncBarrier();

		*(cpu_common.scb + scb_ccr) &= ~(1 << 16);
		hal_cpuDataSyncBarrier();

		ccsidr = *(cpu_common.scb + scb_ccsidr);

		sets = (ccsidr >> 13) & 0x7fff;
		do {
			ways = (ccsidr >> 3) & 0x3ff;
			do {
				*(cpu_common.scb + scb_dcisw) = ((sets & 0x1ff) << 5) | ((ways & 0x3) << 30);
			} while (ways-- != 0);
		} while (sets-- != 0);

		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}
}


__attribute__((section(".noxip"))) void hal_cleanDCache(void)
{
	register u32 ccsidr, sets, ways;

	*(cpu_common.scb + scb_csselr) = 0;

	hal_cpuDataSyncBarrier();
	ccsidr = *(cpu_common.scb + scb_ccsidr);

	/* Clean D$ */
	sets = (ccsidr >> 13) & 0x7fff;
	do {
		ways = (ccsidr >> 3) & 0x3ff;
		do {
			*(cpu_common.scb + scb_dccsw) = ((sets & 0x1ff) << 5) | ((ways & 0x3) << 30);
		} while (ways-- != 0);
	} while (sets-- != 0);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


__attribute__((section(".noxip"))) void hal_cleanDCacheAddr(void *addr, u32 sz)
{
	u32 daddr;
	int dsize;

	if (sz == 0u) {
		return;
	}

	daddr = (((u32)addr) & ~0x1fu);
	dsize = sz + ((u32)addr & 0x1fu);

	hal_cpuDataSyncBarrier();

	do {
		*(cpu_common.scb + scb_dccmvac) = daddr;
		daddr += 0x20u;
		dsize -= 0x20;
	} while (dsize > 0);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


__attribute__((section(".noxip"))) void hal_cleaninvalDCacheAddr(void *addr, u32 sz)
{
	u32 daddr;
	int dsize;

	if (sz == 0u) {
		return;
	}

	daddr = (((u32)addr) & ~0x1fu);
	dsize = sz + ((u32)addr & 0x1fu);

	hal_cpuDataSyncBarrier();

	do {
		*(cpu_common.scb + scb_dccimvac) = daddr;
		daddr += 0x20u;
		dsize -= 0x20;
	} while (dsize > 0);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


__attribute__((section(".noxip"))) void hal_invalDCacheAddr(void *addr, u32 sz)
{
	u32 daddr;
	int dsize;

	if (sz == 0u) {
		return;
	}

	daddr = (((u32)addr) & ~0x1fu);
	dsize = sz + ((u32)addr & 0x1fu);

	hal_cpuDataSyncBarrier();

	do {
		*(cpu_common.scb + scb_dcimvac) = daddr;
		daddr += 0x20u;
		dsize -= 0x20;
	} while (dsize > 0);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


__attribute__((section(".noxip"))) void hal_invalDCacheAll(void)
{
	u32 ccsidr, sets, ways;

	/* select Level 1 data cache */
	*(cpu_common.scb + scb_csselr) = 0;
	hal_cpuDataSyncBarrier();
	ccsidr = *(cpu_common.scb + scb_ccsidr);

	sets = (ccsidr >> 13) & 0x7fff;
	do {
		ways = (ccsidr >> 3) & 0x3ff;
		do {
			*(cpu_common.scb + scb_dcisw) = ((sets & 0x1ff) << 5) | ((ways & 0x3) << 30);
		} while (ways-- != 0U);
	} while (sets-- != 0U);

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


__attribute__((section(".noxip"))) void hal_enableICache(void)
{
	if (!(*(cpu_common.scb + scb_ccr) & (1 << 17))) {
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
		*(cpu_common.scb + scb_iciallu) = 0; /* Invalidate I$ */
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
		*(cpu_common.scb + scb_ccr) |= 1 << 17;
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}
}


__attribute__((section(".noxip"))) void hal_disableICache(void)
{
	if (*(cpu_common.scb + scb_ccr) & (1 << 17)) {
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
		*(cpu_common.scb + scb_ccr) &= ~(1 << 17);
		*(cpu_common.scb + scb_iciallu) = 0;
		hal_cpuDataSyncBarrier();
		hal_cpuInstrBarrier();
	}
}


__attribute__((section(".noxip"))) void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz)
{
	switch (type) {
		case hal_cpuDCache:
			if (sz == (size_t)-1)
				hal_invalDCacheAll();
			else
				hal_invalDCacheAddr((void *)addr, sz);
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
	*(cpu_common.scb + scb_aircr) = ((0x5fa << 16) | (*(cpu_common.scb + scb_aircr) & (0x700)) | (1 << 0x02));
	hal_cpuDataMemoryBarrier();

	for (;;)
		;
}


void hal_cpuInit(void)
{
	cpu_common.scb = (void *)0xe000ed00;

	hal_scbSetPriorityGrouping(3);

	/* Configure cache */
	hal_enableDCache();
	hal_enableICache();

	/* Enable UsageFault, BusFault and MemManage exceptions */
	*(cpu_common.scb + scb_shcsr) |= (1 << 16) | (1 << 17) | (1 << 18);
	hal_cpuDataMemoryBarrier();

	/* Disable deep sleep */
	*(cpu_common.scb + scb_scr) &= ~(1 << 2);
	hal_cpuDataSyncBarrier();
}
