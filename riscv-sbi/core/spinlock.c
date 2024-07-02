/*
 * Phoenix-RTOS
 *
 * Operating system kernel
 *
 * Spinlock
 *
 * Copyright 2018, 2023 Phoenix Systems
 * Author: Pawel Pisarczyk, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "spinlock.h"


void spinlock_set(spinlock_t *spinlock, spinlock_ctx_t *sc)
{
	/* clang-format off */
	__asm__ volatile (
		"csrrc t0, sstatus, (1 << 3)\n\t"
		"sd t0, (%0)\n\t"
		"li t0, 1\n\t"
	"1:\n\t"
		"amoswap.w.aq t0, t0, %1\n\t"
		"bnez t0, 1b\n\t"
		"fence r, rw"
	:
	: "r" (sc), "A" (spinlock->lock)
	: "t0", "memory");
	/* clang-format on */
}


void spinlock_clear(spinlock_t *spinlock, spinlock_ctx_t *sc)
{
	/* clang-format off */
	__asm__ volatile (
		"fence rw, w\n\t"
		"amoswap.w.rl zero, zero, %0\n\t"
		"ld t0, (%1)\n\t"
		"csrw sstatus, t0"
	:
	: "A" (spinlock->lock), "r" (sc)
	: "t0", "memory");
	/* clang-format on */
}


void spinlock_create(spinlock_t *spinlock, const char *name)
{
	spinlock->lock = 0;
	spinlock->name = name;
}
