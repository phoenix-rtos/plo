/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * AArch64 Memory Management Unit (MMU)
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _MMU_H_
#define _MMU_H_

#include <hal/hal.h>

#define SIZE_MMU_SECTION_REGION (1uL << 21)

#define MMU_FLAG_CACHED   0x0
#define MMU_FLAG_UNCACHED 0x1
#define MMU_FLAG_XN       0x2
#define MMU_FLAG_DEVICE   0x4

/* Map 2 MB region to a physical address */
extern void mmu_mapAddr(addr_t paddr, addr_t vaddr, unsigned int flags);


/* Enable MMU and data/instruction caches */
extern void mmu_enable(void);


extern void mmu_disable(void);


extern void mmu_init(void);


#endif
