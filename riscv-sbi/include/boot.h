/*
 * Phoenix SBI
 *
 * Early boot routines
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SBI_BOOT_H_
#define _SBI_BOOT_H_


#define BOOT_WAIT_MAGIC 0x544941575f494253 /* SBI_WAIT */
#define BOOT_DONE_MAGIC 0x454e4f445f494253 /* SBI_DONE */


#ifndef __ASSEMBLY__

#include "types.h"


extern volatile u64 boot_statusArr[];


void boot_release(unsigned int bootHartId);


#endif /* __ASSEMBLY__ */


#endif
