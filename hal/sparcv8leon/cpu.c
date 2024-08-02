/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * LEON CPU related routines
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "hal/hal.h"
#include "gaisler/l2cache.h"


#define CPU_CCR 0x00u /* Cache control register */

/* CCR bits */
#define CCR_FD (1 << 22u)
#define CCR_FI (1 << 21u)


static void hal_cpuSetCCR(u32 mask)
{
	u32 cctrl = hal_cpuLoadAlternate32(CPU_CCR, ASI_CCTRL);
	cctrl |= mask;
	hal_cpuStoreAlternate32(CPU_CCR, ASI_CCTRL, cctrl);
}


void hal_cpuInvCache(unsigned int type, addr_t addr, size_t size)
{
	switch (type) {
		case hal_cpuDCache:
			hal_cpuSetCCR(CCR_FD);
			break;

		case hal_cpuICache:
			hal_cpuSetCCR(CCR_FI);
			break;

		default:
			return;
	}

#ifdef LEON_HAS_L2CACHE
	l2c_flushRange(l2c_inv_line, addr, size);
#endif
}
