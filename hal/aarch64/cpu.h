/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * AArch64 CPU definitions
 *
 * Copyright 2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CPU_H_
#define _CPU_H_


/* AArch64 processor modes */
#define MODE_nAARCH64 0x10
#define MODE_EL0      0x0
#define MODE_EL1_SP0  0x4
#define MODE_EL1_SP1  0x5
#define MODE_MASK     0xf
#define NO_DBGE       0x200             /* mask to disable debug exception */
#define NO_SERR       0x100             /* mask to disable SError exception */
#define NO_IRQ        0x80              /* mask to disable IRQ */
#define NO_FIQ        0x40              /* mask to disable FIQ */
#define NO_INT        (NO_IRQ | NO_FIQ) /* mask to disable IRQ and FIQ */

#ifndef __ASSEMBLY__

#define sysreg_write(sysreg, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		asm volatile( \
				"msr " #sysreg ", %0" \
				: \
				: "r"(__v) \
				: "memory"); \
	})


#define sysreg_read(sysreg) \
	({ \
		register unsigned long __v; \
		asm volatile( \
				"mrs %0, " #sysreg \
				: "=r"(__v) \
				: \
				: "memory"); \
		__v; \
	})


static inline void hal_cpuDataMemoryBarrier(void)
{
	asm volatile("dmb sy");
}


static inline void hal_cpuDataSyncBarrier(void)
{
	asm volatile("dsb ish");
}


static inline void hal_cpuInstrBarrier(void)
{
	asm volatile("isb");
}


static inline void hal_cpuHalt(void)
{
	asm volatile("wfi");
}

#endif

#endif
