/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI IPI handler
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

#include "extensions/ipi.h"


enum {
	IPI_SEND_IPI = 0,
};


static void ecall_ipi_sendIpiHandler(void *data)
{
	(void)data;

	csr_set(CSR_MIP, MIP_SSIP);
}


static sbiret_t ecall_ipi_handler(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid)
{
	sbiret_t ret = { 0 };

	switch (fid) {
		case IPI_SEND_IPI:
			ret.error = sbi_ipiSendMany(a0, a1, ecall_ipi_sendIpiHandler, NULL);
			break;

		default:
			ret.error = SBI_ERR_NOT_SUPPORTED;
			break;
	}

	return ret;
}


static const sbi_ext_t sbi_ext_ipi __attribute__((section("extensions"), used)) = {
	.eid = SBI_EXT_IPI,
	.handler = ecall_ipi_handler,
};
