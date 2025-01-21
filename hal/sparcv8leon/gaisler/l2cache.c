/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * L2 cache management
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "l2cache.h"


#define L2C_MODE_MASK 0x7u
#define L2C_DIS_SHIFT 3u


/* clang-format off */
enum { l2c_ctrl = 0, l2c_status, l2c_fma, l2c_fsi, l2c_err = 8, l2c_erra, l2c_tcb, l2c_dcb, l2c_scrub,
	l2c_sdel, l2c_einj, l2c_accc };
/* clang-format on */


static struct {
	volatile u32 *base;
	size_t lineSz; /* bytes */
} l2c_common;


void l2c_flushRange(unsigned int mode, addr_t start, size_t size)
{
	u32 val;
	addr_t fstart;
	addr_t fend;
	volatile u32 *freg = l2c_common.base + l2c_fma;
	unsigned int disable = ((mode & l2c_disable) != 0) ? 1 : 0;
	mode &= L2C_MODE_MASK;

	/* TN-0021: flush register accesses must be done using atomic operations */

	if ((mode >= l2c_inv_all) && (mode <= l2c_flush_inv_all)) {
		val = mode | (disable << L2C_DIS_SHIFT);
		/* clang-format off */
		__asm__ volatile ("swap [%1], %0" : "+r"(val) : "r"(freg) : "memory");
		/* clang-format on */
	}
	else {
		fstart = start & ~(l2c_common.lineSz - 1);
		fend = fstart + ((((start & (l2c_common.lineSz - 1)) + size) + (l2c_common.lineSz - 1)) & ~(l2c_common.lineSz - 1));

		while (fstart < fend) {
			val = fstart | mode | (disable << L2C_DIS_SHIFT);

			/* Flushing takes 5 cycles/line */

			/* clang-format off */
			__asm__ volatile (
				"swap [%1], %0\n\t"
				 : "+r"(val)
				 : "r"(freg)
				 : "memory"
			);
			/* clang-format on */

			fstart += l2c_common.lineSz;
		}
	}
}


void l2c_init(void *base)
{
	u32 reg;
	size_t lineSz;

	/* Cache should be already configured through startup code */
	l2c_common.base = base;

	reg = *(l2c_common.base + l2c_status);
	lineSz = ((reg & (1 << 24)) != 0) ? 64 : 32;

	l2c_common.lineSz = lineSz;
}
