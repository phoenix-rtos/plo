/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR712RC specific functions
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _GR712RC_H_
#define _GR712RC_H_

#include "../types.h"


#define ASI_MMU_BYPASS 0x1c


/* clang-format off */

/* Clock gating unit devices */

enum { cgudev_eth = 0, cgudev_spw0, cgudev_spw1, cgudev_spw2, cgudev_spw3, cgudev_spw4, cgudev_spw5, cgudev_can,
	cgudev_ccsdsEnc = 9, cgudev_ccsdsDec, cgudev_milStd1553 };

/* clang-format on */


static inline void hal_cpuHalt(void)
{
	/* GR712RC errata 1.7.8 */
	u32 addr = 0xfffffff0u;

	/* clang-format off */

	__asm__ volatile(
		"wr %%g0, %%asr19\n\t"
		"lda [%0] %c1, %%g0\n\t"
		:
		: "r"(addr), "i"(ASI_MMU_BYPASS)
	);
	/* clang-format on */
}


void _gr712rc_cguClkEnable(u32 device);


void _gr712rc_cguClkDisable(u32 device);


int _gr712rc_cguClkStatus(u32 device);


void _gr712rc_init(void);


#endif
