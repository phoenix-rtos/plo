/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv8-A cache management
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#include "cache.h"


static u64 getL1DcacheID(void)
{
	sysreg_write(csselr_el1, 0); /* Select L1 Dcache */
	return sysreg_read(ccsidr_el1);
}


void hal_dcacheInvalAll(void)
{
	u64 cacheSizeID = getL1DcacheID();
	u64 lineSizeLog = (cacheSizeID & 0x7) + 4;
	u64 assoc = (cacheSizeID >> 3) & 0x3ff;
	u64 sets = (cacheSizeID >> 13) & 0x7fff;
	u32 assocLog = __builtin_clz((u32)assoc);
	u64 wayCtr, setCtr, setway;

	hal_cpuDataSyncBarrier();
	for (wayCtr = 0; wayCtr <= assoc; wayCtr++) {
		for (setCtr = 0; setCtr <= sets; setCtr++) {
			setway = (wayCtr << assocLog) | (setCtr << lineSizeLog);
			asm volatile("dc cisw, %0" : : "r"(setway) : "memory");
		}
	}

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


void hal_icacheInval(void)
{
	asm volatile(
			"dsb ish\n"
			"ic iallu\n"
			"dsb ish\n"
			"isb\n");
}


static u64 hal_getCacheLineSize(void)
{
	u64 ctr = sysreg_read(ctr_el0);
	ctr = (ctr >> 16) & 0xf;
	return 4 << ctr;
}


void hal_dcacheClean(addr_t start, addr_t end)
{
	u64 ctr;
	hal_cpuDataSyncBarrier();
	ctr = hal_getCacheLineSize();
	start &= ~(ctr - 1);
	while (start < end) {
		asm volatile("dc cvac, %0" : : "r"(start) : "memory");
		start += ctr;
	}

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


void hal_dcacheInval(addr_t start, addr_t end)
{
	u64 ctr;
	hal_cpuDataSyncBarrier();
	ctr = hal_getCacheLineSize();
	start &= ~(ctr - 1);
	while (start < end) {
		asm volatile("dc ivac, %0" : : "r"(start) : "memory");
		start += ctr;
	}

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


void hal_dcacheFlush(addr_t start, addr_t end)
{
	u64 ctr;
	hal_cpuDataSyncBarrier();
	ctr = hal_getCacheLineSize();
	start &= ~(ctr - 1);
	while (start < end) {
		asm volatile("dc civac, %0" : : "r"(start) : "memory");
		start += ctr;
	}

	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


static void cacheToggle(unsigned int mode, u64 sctlr_bit)
{
	asm volatile(
			"dsb ish\n"
			"mrs x2, sctlr_el3\n"
			"bic x2, x2, %0\n"
			"cmp %1, #0\n"
			"csel %0, %0, xzr, ne\n"
			"orr x2, x2, %0\n"
			"msr sctlr_el3, x2\n"
			"dsb ish\n"
			"isb\n"
			: "+r"(sctlr_bit) : "r"(mode) : "x2", "memory");
}


void hal_dcacheEnable(unsigned int mode)
{
	return cacheToggle(mode, 1 << 2);
}


void hal_icacheEnable(unsigned int mode)
{
	return cacheToggle(mode, 1 << 12);
}
