/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * CPU related routines
 *
 * Copyright 2012, 2017, 2021 Phoenix Systems
 * Copyright 2001, 2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CPU_H_
#define _CPU_H_


#include "types.h"


static inline void hal_cpuDataMemoryBarrier(void)
{
	/* not supported */
}


static inline void hal_cpuDataSyncBarrier(void)
{
	/* not supported */
}


static inline void hal_cpuInstrBarrier(void)
{
	/* not supported */
}


static inline u8 hal_inb(u16 addr)
{
	u8 b;

	/* clang-format off */
	__asm__ volatile (
		"inb %1, %0\n\t"
	: "=a" (b)
	: "d" (addr)
	: );
	/* clang-format on */
	return b;
}


static inline void hal_outb(u16 addr, u8 b)
{
	/* clang-format off */
	__asm__ volatile (
		"outb %1, %0"
	:
	: "d" (addr), "a" (b)
	: );
	/* clang-format on */

	return;
}


static inline u16 hal_inw(u16 addr)
{
	u16 w;

	/* clang-format off */
	__asm__ volatile (
		"inw %1, %0\n\t"
	: "=a" (w)
	: "d" (addr)
	: );
	/* clang-format on */

	return w;
}


static inline void hal_outw(u16 addr, u16 w)
{
	/* clang-format off */
	__asm__ volatile (
		"outw %1, %0"
	:
	: "d" (addr), "a" (w)
	: );
	/* clang-format on */

	return;
}


static inline u32 hal_inl(u16 addr)
{
	u32 l;

	/* clang-format off */
	__asm__ volatile (
		"inl %1, %0\n\t"
	: "=a" (l)
	: "d" (addr)
	: );
	/* clang-format on */

	return l;
}


static inline void hal_outl(u16 addr, u32 l)
{
	/* clang-format off */
	__asm__ volatile (
		"outl %1, %0"
	:
	: "d" (addr), "a" (l)
	: );
	/* clang-format on */

	return;
}


extern void hal_cpuHalt(void);


#endif
