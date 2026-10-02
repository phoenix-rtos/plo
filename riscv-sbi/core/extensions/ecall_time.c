/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI TIMER handler
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "csr.h"
#include "sbi.h"

#include "devices/clint.h"


enum {
	TIME_SET_TIMER = 0,
};


static sbiret_t ecall_time_handler(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid)
{
	sbiret_t ret = {
		.error = SBI_SUCCESS,
		.value = 0,
	};

	switch (fid) {
		case TIME_SET_TIMER:
#ifdef __riscv_sstc
			/* With Sstc STIP is driven by stimecmp */
			csr_write(CSR_STIMECMP, a0);
#else
			clint_setTimecmp(a0);
#endif
			break;

		default:
			ret.error = SBI_ERR_NOT_SUPPORTED;
			break;
	}

	return ret;
}


static const sbi_ext_t sbi_ext_time __attribute__((section("extensions"), used)) = {
	.eid = SBI_EXT_TIME,
	.handler = ecall_time_handler,
};
