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

#ifndef _MMU_H_
#define _MMU_H_

#include <hal/hal.h>

#define SIZE_MMU_SECTION_REGION 0x100000

#define MMU_FLAG_CACHED   0x1
#define MMU_FLAG_UNCACHED 0x2
#define MMU_FLAG_XN       0x4

/* Mapping 1 MB region to a physical address */
extern void mmu_mapAddr(addr_t paddr, addr_t vaddr, unsigned int flags);


extern void mmu_enable(void);


extern void mmu_disable(void);


extern void mmu_init(void);


#endif
