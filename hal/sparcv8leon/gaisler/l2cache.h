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

#ifndef _L2CACHE_H_
#define _L2CACHE_H_


#include <types.h>


/* clang-format off */
enum { l2c_inv_line = 1, l2c_flush_line, l2c_flush_inv_line, l2c_inv_all = 5, l2c_flush_all, l2c_flush_inv_all, l2c_disable = 0x10 };
/* clang-format on */


void l2c_flushRange(unsigned int mode, addr_t start, size_t size);


void l2c_init(void *base);


#endif
