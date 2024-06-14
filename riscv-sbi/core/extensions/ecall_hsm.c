/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI HSM handler
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "sbi.h"

#include "extensions/hsm.h"


enum {
	HSM_HART_START = 0,
	HSM_HART_STOP,
	HSM_HART_GET_STATUS,
	HSM_HART_SUSPEND,
};


static sbiret_t ecall_hsm_handler(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid)
{
	sbiret_t ret = { 0 };

	switch (fid) {
		case HSM_HART_START:
			ret.error = hsm_hartStart(a0, a1, a2);
			break;

		case HSM_HART_GET_STATUS:
			ret = hsm_hartGetStatus(a0);
			break;

		case HSM_HART_STOP:
		case HSM_HART_SUSPEND:
		default:
			ret.error = SBI_ERR_NOT_SUPPORTED;
			break;
	}

	return ret;
}


static const sbi_ext_t sbi_ext_hsm __attribute__((section("extensions"), used)) = {
	.eid = SBI_EXT_HSM,
	.handler = ecall_hsm_handler,
};
