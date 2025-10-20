/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * CSR definitions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_CSR_H_
#define _SBI_CSR_H_


#ifndef __ASSEMBLY__


#include "types.h"


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


int csr_emulateRead(u32 csr, u64 *val);


int csr_emulateWrite(u32 csr, u64 val);


#endif /* __ASSEMBLY__ */


/* Interrupt numbers */

#define IRQ_S_SOFT  1
#define IRQ_M_SOFT  3
#define IRQ_S_TIMER 5
#define IRQ_M_TIMER 7
#define IRQ_S_EXT   9
#define IRQ_M_EXT   11
#define IRQ_PMU_OVF 13

/* Unprivileged FP CSRs */

#define CSR_FFLAGS 0x001U
#define CSR_FRM    0x002U
#define CSR_FCSR   0x003U

/* Unprivileged CSRs */

#define CSR_CYCLE   0xc00u
#define CSR_TIME    0xc01u
#define CSR_INSTRET 0xc02u

/* Supervisor CSRs */

#define CSR_SSTATUS    0x100u
#define CSR_SIE        0x104u
#define CSR_STVEC      0x105u
#define CSR_SCOUNTEREN 0x106u
#define CSR_SSCRATCH   0x140u
#define CSR_SATP       0x180u

#define CSR_SEPC   0x141u
#define CSR_SCAUSE 0x142u
#define CSR_STVAL  0x143u
#define CSR_SIP    0x144u

/* Machine CSRs */

#define CSR_MSTATUS    0x300u
#define CSR_MISA       0x301u
#define CSR_MEDELEG    0x302u
#define CSR_MIDELEG    0x303u
#define CSR_MIE        0x304u
#define CSR_MTVEC      0x305u
#define CSR_MCOUNTEREN 0x306u
#define CSR_MENVCFG    0x30au

#define CSR_MSCRATCH 0x340u
#define CSR_MEPC     0x341u
#define CSR_MCAUSE   0x342u
#define CSR_MTVAL    0x343u
#define CSR_MIP      0x344u
#define CSR_MTINST   0x34au

#define CSR_MCYCLE   0xb00u
#define CSR_MINSTRET 0xb02u

#define CSR_MVENDORID 0xf11u
#define CSR_MARCHID   0xf12u
#define CSR_MIMPID    0xf13u
#define CSR_MHARTID   0xf14u

/* Machine memory protection */

#define CSR_PMPCFG0  0x3a0u
#define CSR_PMPADDR0 0x3b0u

/* CSR bits */

#define MSTATUS_MPP_SHIFT 11

#define MSTATUS_SIE  (1UL << 1)
#define MSTATUS_MIE  (1UL << 3)
#define MSTATUS_SPIE (1UL << 5)
#define MSTATUS_MPIE (1UL << 7)
#define MSTATUS_SPP  (1UL << 8)
#define MSTATUS_MPP  (3UL << MSTATUS_MPP_SHIFT)

#define MIE_MSIE (1UL << 3)

#define MIP_SSIP (1UL << IRQ_S_SOFT)
#define MIP_MSIP (1UL << IRQ_M_SOFT)
#define MIP_STIP (1UL << IRQ_S_TIMER)
#define MIP_MTIP (1UL << IRQ_M_TIMER)
#define MIP_SEIP (1UL << IRQ_S_EXT)
#define MIP_MEIP (1UL << IRQ_M_EXT)

#define MCAUSE_IRQ_MSK 0xffUL

#define MCAUSE_ILLEGAL 0x2UL
#define MCAUSE_S_ECALL 0x9UL
#define MCAUSE_INTR    (1UL << 63)

/* Privilege levels */

#define PRV_U 0UL
#define PRV_S 1UL
#define PRV_M 3UL


#endif
