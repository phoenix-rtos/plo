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


static inline unsigned char hal_inb(void *addr)
{
	unsigned char val;

	__asm__ volatile(
		"inb %1, %0; "
	: "=a" (val)
	: "Nd" (addr));

	return val;
}


static inline void hal_outb(void *addr, unsigned char val)
{
	__asm__ volatile(
		"outb %0, %1; "
	:: "a" (val), "Nd" (addr));
}


static inline unsigned short hal_inw(void *addr)
{
	unsigned short val;

	__asm__ volatile(
		"inw %1, %0; "
	: "=a" (val)
	: "Nd" (addr));

	return val;
}


static inline void hal_outw(void *addr, unsigned short val)
{
	__asm__ volatile(
		"outw %0, %1; "
	:: "a" (val), "Nd" (addr));
}


static inline unsigned int hal_inl(void *addr)
{
	unsigned int val;

	__asm__ volatile(
		"inl %1, %0; "
	: "=a" (val)
	: "Nd" (addr));

	return val;
}


static inline void hal_outl(void *addr, unsigned int val)
{
	__asm__ volatile(
		"outl %0, %1; "
	:: "a" (val), "Nd" (addr));
}


extern void hal_cpuHalt(void);


#endif
