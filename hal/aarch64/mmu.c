/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * AArch64 Memory Management Unit (MMU)
 *
 * Copyright 2024 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "mmu.h"

/* Descriptor bitfields */
#define DESCR_VALID   (1uL << 0)         /* Descriptor is valid */
#define DESCR_TABLE   (1uL << 1)         /* Page or table descriptor */
#define DESCR_ATTR(x) (((x) & 0x7) << 2) /* Memory attribute from MAIR_EL1 */
#define DESCR_AP1     (1uL << 6)         /* Unprivileged access */
#define DESCR_AP2     (1uL << 7)         /* Read only */
#define DESCR_nSH     (0uL << 8)         /* Non-shareable */
#define DESCR_OSH     (2uL << 8)         /* Outer shareable */
#define DESCR_ISH     (3uL << 8)         /* Inner shareable */
#define DESCR_AF      (1uL << 10)        /* Access flag */
#define DESCR_nG      (1uL << 11)        /* Not global */
#define DESCR_UXN     (1uL << 54)        /* Unprivileged execute-never */
#define DESCR_PXN     (1uL << 53)        /* Privileged execute-never */

/* MAIR register bitfields */
#define MAIR_ATTR(idx, val)       (((u64)val) << (idx * 8))
#define MAIR_DEVICE(type)         (((type) & 0x3) << 2)
#define MAIR_NORMAL(inner, outer) (((inner) & 0xf) | (((outer) & 0xf) << 4))
#define MAIR_DEV_nGnRnE           0x0 /* Gathering, re-ordering, early write acknowledge all disabled */
#define MAIR_DEV_nGnRE            0x1 /* Gathering, re-ordering disabled, early write acknowledge enabled */
#define MAIR_NOR_NC               0x4 /* Non-cacheable */
#define MAIR_NOR_C_WB_NT_RA_WA    0xf /* Cacheable, write-back, non-transient, read-allocate, write-allocate */

#define MAIR_IDX_CACHED    0
#define MAIR_IDX_NONCACHED 1
#define MAIR_IDX_DEVICE    2
#define MAIR_IDX_S_ORDERED 3

/* Translation table has to be aligned to its size, but at least 64 bytes */
static u64 ttl1[4] __attribute__((aligned(64)));
static u64 ttl2[4][512] __attribute__((aligned(SIZE_PAGE)));


static inline void mmu_invalTLB(void)
{
	hal_cpuDataSyncBarrier();
	/* clang-format off */
	asm volatile ("tlbi vmalle1");
	/* clang-format on */
	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


static inline void mmu_setTranslationRegs(u64 ttbr0, u64 tcr, u64 mair)
{
	hal_cpuDataSyncBarrier();
	sysreg_write(ttbr0_el1, ttbr0);
	sysreg_write(tcr_el1, tcr);
	sysreg_write(mair_el1, mair);
	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


void mmu_enable(void)
{
	u64 val;
	hal_cpuDataSyncBarrier();
	val = sysreg_read(sctlr_el1);
	val |= ((1 << 12) | (1 << 2) | (1 << 0));
	sysreg_write(sctlr_el1, val);
	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
}


void mmu_disable(void)
{
	u64 val;
	hal_cpuDataSyncBarrier();
	val = sysreg_read(sctlr_el1);
	val &= ~((1 << 12) | (1 << 2) | (1 << 0));
	sysreg_write(sctlr_el1, val);
	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
	mmu_invalTLB();
}


void mmu_mapAddr(addr_t paddr, addr_t vaddr, unsigned int flags)
{
	u64 idx1 = (vaddr >> 30) & ((1uL << 2) - 1);
	u64 idx2 = (vaddr >> 21) & ((1uL << 9) - 1);
	u64 descriptor = (paddr & ~(SIZE_MMU_SECTION_REGION - 1)) | DESCR_ISH | DESCR_AF | DESCR_VALID;

	if ((flags & MMU_FLAG_XN) != 0) {
		descriptor |= DESCR_PXN;
	}

	if ((flags & MMU_FLAG_DEVICE) != 0) {
		descriptor |= DESCR_ATTR(MAIR_IDX_DEVICE);
	}
	else if ((flags & MMU_FLAG_UNCACHED) != 0) {
		descriptor |= DESCR_ATTR(MAIR_IDX_NONCACHED);
	}
	else {
		descriptor |= DESCR_ATTR(MAIR_IDX_CACHED);
	}


	ttl2[idx1][idx2] = descriptor;

	hal_cpuDataSyncBarrier();
	hal_cpuDataSyncBarrier();
	hal_cpuInstrBarrier();
	/* clang-format off */
	asm volatile ("tlbi vaae1, %0" :: "r"(vaddr));
	/* clang-format on */
}


void mmu_init(void)
{
	int i;
	addr_t addr, idx1, idx2;
	u64 ttbr0, tcr, mair;

	for (i = 0; i < sizeof(ttl1) / sizeof(ttl1[0]); i++) {
		ttl1[i] = ((addr_t)ttl2[i]) | DESCR_VALID | DESCR_TABLE;
	}

	/* Map all memory as device, execute-never by default */
	for (addr = 0; addr < (1uL << 32); addr += SIZE_MMU_SECTION_REGION) {
		idx1 = (addr >> 30) & ((1uL << 2) - 1);
		idx2 = (addr >> 21) & ((1uL << 9) - 1);
		ttl2[idx1][idx2] = (addr & ~(SIZE_MMU_SECTION_REGION - 1)) |
				DESCR_PXN | DESCR_ATTR(MAIR_IDX_DEVICE) |
				DESCR_ISH | DESCR_AF | DESCR_VALID;
	}

	ttbr0 = (addr_t)ttl1;
	/* Read physical address range from ID register, then use it for TCR register */
	tcr = sysreg_read(id_aa64mmfr0_el1);
	tcr = (tcr & 0x7) << 32;
	tcr |=
			(1uL << 23) | /* Disable translation through TTBR1 */
			(0uL << 14) | /* 4 KB granule */
			(3uL << 12) | /* Inner shareable */
			(1uL << 10) | /* Outer cacheable */
			(1uL << 8) |  /* Inner cacheable */
			(0uL << 7) |  /* Enable translation through TTBR0 */
			32;           /* 4 GB virtual address range */

	mair =
			MAIR_ATTR(MAIR_IDX_CACHED, MAIR_NORMAL(MAIR_NOR_C_WB_NT_RA_WA, MAIR_NOR_C_WB_NT_RA_WA)) |
			MAIR_ATTR(MAIR_IDX_NONCACHED, MAIR_NORMAL(MAIR_NOR_NC, MAIR_NOR_NC)) |
			MAIR_ATTR(MAIR_IDX_DEVICE, MAIR_DEVICE(MAIR_DEV_nGnRE)) |
			MAIR_ATTR(MAIR_IDX_S_ORDERED, MAIR_DEVICE(MAIR_DEV_nGnRnE));

	mmu_setTranslationRegs(ttbr0, tcr, mair);
}
