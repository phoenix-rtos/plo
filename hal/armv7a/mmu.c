/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv7 Cortex-A Memory Management Unit (MMU)
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "mmu.h"


static u32 ttl1[0x1000] __attribute__((section(".uncached_ddr"), aligned(0x4000)));


static inline void mmu_invalTLB(void)
{
	__asm__ volatile (" \
		dsb; \
		mcr p15, 0, r1, c8, c7, 0; \
		dsb; \
		isb");
}


static inline void mmu_setTTBR0(addr_t addr)
{
	__asm__ volatile (" \
		dsb; \
		mov r1, #1; \
		mcr p15, 0, r1, c2, c0, 2; \
		mcr p15, 0, %0, c2, c0, 0; \
		mcr p15, 0, %0, c2, c0, 1; \
		dsb; \
		isb"
		:
		: "r" (addr)
		: "r1");
}


static inline void mmu_setDACR(u32 val)
{
	__asm__ volatile (" \
		dsb; \
		mcr p15, 0, %0, c3, c0, 0; \
		dsb; \
		isb"
		:
		: "r" (val)
		: );
}


void mmu_enable(void)
{
	__asm__ volatile (" \
		dsb; \
		mrc p15, 0, r1, c1, c0, 0; \
		orr r1, r1, #1; \
		mcr p15, 0, r1, c1, c0, 0; \
		dsb; \
		isb"
		: : : "r1"
	);
}


void mmu_disable(void)
{
	__asm__ volatile (" \
		dsb; \
		mrc p15, 0, r1, c1, c0, 0; \
		bic r1, r1, #1; \
		mcr p15, 0, r1, c1, c0, 0; \
		dsb; \
		isb"
		: : : "r1"
	);

	mmu_invalTLB();
}


void mmu_mapAddr(addr_t paddr, addr_t vaddr, unsigned int flags)
{
	unsigned int id, tex = 0, ap = 0, xn = 0, cb = 0;

	/* Full access read/write */
	ap = 0x3;

	if (flags & MMU_FLAG_UNCACHED) {
		tex = 2;
		cb = 0; /* C = 0 & B =0 */
	}

	if (flags & MMU_FLAG_CACHED) {
		tex = 6;
		cb = 0x3; /* C = 1 & B = 1 */
	}

	if (flags & MMU_FLAG_XN) {
		tex = 0;
		xn = 1;
	}

	id = vaddr >> 20;
	ttl1[id] = (paddr & ~(SIZE_MMU_SECTION_REGION - 1)) | (tex << 12) | (ap << 10) | (xn << 4) | (cb << 2) | 0x2;

	mmu_invalTLB();
}


void mmu_init(void)
{
	int i;
	addr_t ttbr0, addr;

	hal_memset(ttl1, 0, sizeof(ttl1));

	/* Set generic attributes for all memory maps as uncached */
	for (i = 0; i < sizeof(ttl1) / sizeof(ttl1[0]); ++i) {
		addr = (addr_t)(SIZE_MMU_SECTION_REGION) * (addr_t)(i);
		mmu_mapAddr(addr, addr, MMU_FLAG_UNCACHED | MMU_FLAG_XN);
	}

	/* Inner cacheability, Outer cacheability  */
	ttbr0 = (addr_t)ttl1 | (1 << 6) | (3 << 3) | 1;

	mmu_setTTBR0(ttbr0);
	mmu_setDACR(0x00000001);
}
