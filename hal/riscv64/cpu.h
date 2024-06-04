/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * RV64 CPU related routines
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CPU_H_
#define _CPU_H_


/* CSR bits */
#define SSTATUS_SIE (1u << 1)
#define SSTATUS_FS  (3u << 13)

#define SCAUSE_IRQ_MSK (0xff)
#define SCAUSE_INTR    (1u << 63)

/* Interrupts */
#define CLINT_IRQ_FLG (1u << 31) /* Marks that interrupt handler is installed for CLINT, not PLIC */

#ifndef __ASSEMBLY__


#include "sbi.h"


/* clang-format off */

#define csr_set(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ volatile ( \
			"csrs %0, %1" \
			:: "i"(csr), "rK"(__v) \
			: "memory"); \
		__v; \
	})


#define csr_write(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ volatile ( \
			"csrw %0, %1" \
			:: "i"(csr), "rK"(__v) \
			: "memory"); \
	})


#define csr_clear(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ volatile ( \
			"csrc %0, %1" \
			::"i"(csr), "rK"(__v) \
			: "memory"); \
	})


#define csr_read(csr) \
	({ \
		register unsigned long __v; \
		__asm__ volatile ( \
			"csrr %0, %1" \
			: "=r"(__v) \
			: "i"(csr) \
			:"memory"); \
		__v; \
	})

/* clang-format on */

static inline void hal_cpuHalt(void)
{
	__asm__ volatile("wfi");
}


static inline void hal_cpuDataStoreBarrier(void)
{
	__asm__ volatile("fence");
}


static inline void hal_cpuDataMemoryBarrier(void)
{
	__asm__ volatile("fence");
}


static inline void hal_cpuDataSyncBarrier(void)
{
	__asm__ volatile("fence");
}


static inline void hal_cpuInstrBarrier(void)
{
	__asm__ volatile("fence.i");
}


static inline void hal_cpuReboot(void)
{
	sbi_reset(SBI_RESET_TYPE_COLD, SBI_RESET_REASON_NONE);
}


static inline unsigned long hal_cpuGetHartId(void)
{
	unsigned long id;
	/* clang-format off */
	__asm__ volatile ("mv %0, tp" : "=r"(id));
	/* clang-format on */
	return id;
}


#endif /* __ASSEMBLY__ */


#endif /* _CPU_H_ */
