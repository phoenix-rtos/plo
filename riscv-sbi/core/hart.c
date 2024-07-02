/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * HART features
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "csr.h"
#include "hart.h"


void __attribute__((noreturn)) hart_halt(void)
{
	while (1) {
		__WFI();
	}
	__builtin_unreachable();
}


void __attribute__((noreturn)) hart_changeMode(sbi_param arg0, sbi_param arg1, addr_t nextAddr, sbi_param nextMode)
{
	unsigned long v = csr_read(CSR_MSTATUS);

	v = (v & ~(MSTATUS_MPP | MSTATUS_MIE)) | (nextMode << 11);

	csr_write(CSR_MSTATUS, v);
	csr_write(CSR_MEPC, nextAddr);
	csr_write(CSR_STVEC, nextAddr);
	csr_write(CSR_SSCRATCH, 0);
	csr_write(CSR_SIE, 0);
	csr_write(CSR_SATP, 0);

	/* clang-format off */
	__asm__ volatile(
		"mv a0, %0\n\t"
		"mv a1, %1\n\t"
		"mret"
		:
		: "r"(arg0), "r"(arg1)
		: "a0", "a1"
	);
	/* clang-format on */

	__builtin_unreachable();
}


void hart_init(void)
{
	/* Enable counters for supervisor */
	csr_write(CSR_MCOUNTEREN, -1);

	/* Enable IR, TM, CY counters for user */
	csr_write(CSR_SCOUNTEREN, 0x7);

	/* Delegate supervisor software, timer and external interrupts to S-Mode */
	csr_write(CSR_MIDELEG, 0x222);

	/* Delegate most exceptions to S-Mode
	 * Change this if we want to emulate some functionality in M-Mode
	 */
	csr_write(CSR_MEDELEG, 0xb1fb);

	/* Enable IPI */
	csr_set(CSR_MIE, MIP_MSIP);
}
