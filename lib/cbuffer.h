/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Circular buffer - code partially derived from phoenix-rtos-kernel/lib/cbuffer.*
 *
 * Copyright 2019, 2021 Phoenix Systems
 * Author: Jan Sikorski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIB_CBUFFER_H_
#define _LIB_CBUFFER_H_

#include "lib.h"
#include <hal/hal.h>

typedef struct {
	size_t capacity;
	volatile size_t tail, head;
	volatile u8 full;

	void *data;
} cbuffer_t;


extern size_t lib_cbufSize(const cbuffer_t *buf);


extern int lib_cbufEmpty(const cbuffer_t *buf);


extern size_t lib_cbufRead(cbuffer_t *buf, void *data, size_t sz);


extern size_t lib_cbufWrite(cbuffer_t *buf, const void *data, size_t sz);


extern void lib_cbufInit(cbuffer_t *buf, void *data, size_t capacity);


#endif
