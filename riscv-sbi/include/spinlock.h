/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * Spinlock
 *
 * Copyright 2014, 2017 Phoenix Systems
 * Author: Jacek Popko, Pawel Pisarczyk, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_SPINLOCK_H_
#define _HAL_SPINLOCK_H_


#include "types.h"


typedef u64 spinlock_ctx_t;

typedef struct _spinlock_t {
	const char *name;
	u32 lock;
} spinlock_t;


void spinlock_set(spinlock_t *spinlock, spinlock_ctx_t *sc);


void spinlock_clear(spinlock_t *spinlock, spinlock_ctx_t *sc);


void spinlock_create(spinlock_t *spinlock, const char *name);


#endif
