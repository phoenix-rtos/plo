/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * Atomic operations
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_ATOMIC_H_
#define _SBI_ATOMIC_H_


#include "hart.h"
#include "types.h"


static inline u64 atomic_cas64(vu64 *ptr, u64 old, u64 new)
{
	u64 prev;

	/* clang-format off */
	__asm__ volatile (
	"1:\n\t"
		"lr.d.aqrl %0, (%2)\n\t"
		"bne %0, %3, 2f\n\t"
		"sc.d.rl %1, %4, (%2)\n\t"
		"bnez %1, 1b\n\t"
	"2:\n\t"
		: "=&r" (prev), "=&r" (old), "+r" (ptr)
		: "r" (old), "r" (new)
		: "memory"
	);
	/* clang-format on */

	return prev;
}


static inline void atomic_add32(vu32 *ptr, u32 val)
{
	/* clang-format off */
	__asm__ volatile (
		"amoadd.w.aq zero, %1, (%0)"
		: "+r" (ptr)
		: "r" (val)
		: "memory"
	);
	/* clang-format on */
}


static inline void atomic_add64(vu64 *ptr, u64 val)
{
	/* clang-format off */
	__asm__ volatile (
		"amoadd.d.aq zero, %1, (%0)"
		: "+r" (ptr)
		: "r" (val)
		: "memory"
	);
	/* clang-format on */
}


#define ATOMIC_READ(ptr) ({ \
	typeof(*ptr) ret = *ptr; \
	RISCV_FENCE(ir, ir); \
	ret; \
})


#define ATOMIC_WRITE(ptr, val) ({ \
	*ptr = val; \
	RISCV_FENCE(ow, ow); \
})


#endif
