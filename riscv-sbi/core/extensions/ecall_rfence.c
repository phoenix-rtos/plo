/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI RFENCE handler
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "sbi.h"


enum {
	RFENCE_FENCE_I = 0,
	RFENCE_SFENCE_VMA,
	RFENCE_SFENCE_VMA_ASID,
	RFENCE_HFENCE_GVMA_VMID,
	RFENCE_HFENCE_GVMA,
	RFENCE_HFENCE_VVMA_ASID,
	RFENCE_HFENCE_VVMA
};


static sbiret_t ecall_rfence_handler(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid)
{
	sbiret_t ret = { 0 };

	/* TODO */
	switch (fid) {
		case RFENCE_FENCE_I:
			ret.error = SBI_ERR_NOT_SUPPORTED;
			break;

		case RFENCE_SFENCE_VMA:
			ret.error = SBI_ERR_NOT_SUPPORTED;
			break;

		case RFENCE_SFENCE_VMA_ASID:
			ret.error = SBI_ERR_NOT_SUPPORTED;
			break;

		case RFENCE_HFENCE_GVMA_VMID:
		case RFENCE_HFENCE_GVMA:
		case RFENCE_HFENCE_VVMA_ASID:
		case RFENCE_HFENCE_VVMA:
		default:
			ret.error = SBI_ERR_NOT_SUPPORTED;
			break;
	}

	return ret;
}


static const sbi_ext_t sbi_ext_rfence __attribute__((section("extensions"), used)) = {
	.eid = SBI_EXT_RFENCE,
	.handler = ecall_rfence_handler,
};
