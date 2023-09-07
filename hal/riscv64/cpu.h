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

/* clang-format off */

#define csr_set(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ volatile("csrs " #csr ", %0" ::"rK"(__v) : "memory"); \
		__v; \
	})


#define csr_write(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ volatile("csrw " #csr ", %0" ::"rK"(__v) : "memory"); \
	})


#define csr_clear(csr, val) \
	({ \
		unsigned long __v = (unsigned long)(val); \
		__asm__ volatile("csrc " #csr ", %0" ::"rK"(__v) : "memory"); \
	})


#define csr_read(csr) \
	({ \
		register unsigned long __v; \
		__asm__ volatile("csrr %0, " #csr : "=r"(__v)::"memory"); \
		__v; \
	})

/* clang-format on */

static inline void hal_cpuHalt(void)
{
	__asm__ volatile("wfi");
}


static inline void hal_cpuDataStoreBarrier(void)
{
}


static inline void hal_cpuDataMemoryBarrier(void)
{
}


static inline void hal_cpuDataSyncBarrier(void)
{
}


static inline void hal_cpuInstrBarrier(void)
{
}


static inline void hal_cpuReboot(void)
{
	/* TODO */
	for (;;) {
	}
}

#endif /* __ASSEMBLY__ */


#endif /* _CPU_H_ */
