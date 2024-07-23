/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv8 Cortex-R cache management
 *
 * Copyright 2022, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CACHE_H_
#define _CACHE_H_


#include "hal/armv8r/types.h"


void hal_dcacheEnable(unsigned int mode);


void hal_dcacheInval(addr_t start, addr_t end);


void hal_dcacheFlush(addr_t start, addr_t end);


void hal_icacheEnable(unsigned int mode);


void hal_icacheInval(void);


#endif
