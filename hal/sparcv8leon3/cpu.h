/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Leon3 CPU related routines
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CPU_H_
#define _CPU_H_


/* Processor State Register */
#define PSR_CWP 0x1f       /* Current window pointer */
#define PSR_ET  (1 << 5)   /* Enable traps */
#define PSR_PS  (1 << 6)   /* Previous supervisor */
#define PSR_S   (1 << 7)   /* Supervisor */
#define PSR_PIL (0xf << 8) /* Processor interrupt level */
#define PSR_EF  (1 << 12)  /* Enable floating point */
#define PSR_EC  (1 << 13)  /* Enable co-processor */
#define PSR_C   (1 << 20)  /* Carry bit */
#define PSR_V   (1 << 21)  /* Overflow bit */
#define PSR_Z   (1 << 22)  /* Zero bit */
#define PSR_N   (1 << 23)  /* Negative bit */

/* Trap Base Register */
#define TBR_TT_MSK   0xFF0
#define TBR_TT_SHIFT 4


#ifndef __ASSEMBLY__

#include <types.h>
#include <peripherals.h>


static inline void hal_cpuHalt(void)
{
	/* must be performed in supervisor mode with int enabled */
	__asm__ volatile("wr %g0, %asr19");
}


static inline void hal_cpuDataStoreBarrier(void)
{
	__asm__ volatile("stbar");
}


static inline void hal_cpuReset(void)
{
	/* TODO */
	*(u32 *)(INT_CTRL_BASE + 128) = 0x00000001;
}


#endif /* __ASSEMBLY__ */


#endif /* _CPU_H_ */
