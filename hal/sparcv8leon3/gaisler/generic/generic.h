/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * LEON3 Generic specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _GENERIC_H_
#define _GENERIC_H_

#include "../types.h"


static inline void hal_cpuHalt(void)
{
	/* clang-format off */
	__asm__ volatile ("wr %g0, %asr19");
	/* clang-format on */
}


#endif
