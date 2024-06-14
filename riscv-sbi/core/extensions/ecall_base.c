/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI BASE handler
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "sbi.h"
#include "csr.h"


#define SBI_VERSION_MAJOR 2
#define SBI_VERSION_MINOR 0

#define SBI_VERSION(major, minor) (((((major)) & 0x7f) << 24) | (minor & 0xffffff))


#define PHOENIX_SBI_ID      10
#define PHOENIX_SBI_VERSION 1


/* Anchors for section which contains extension entries */
extern const sbi_ext_t __ext_start[];
extern const sbi_ext_t __ext_end[];


enum {
	BASE_GET_SPEC_VERSION = 0,
	BASE_GET_IMPL_ID,
	BASE_GET_IMPL_VERSION,
	BASE_PROBE_EXT,
	BASE_GET_MVENDORID,
	BASE_GET_MARCHID,
	BASE_GET_MIMPID,
};


static long ecall_base_probeExt(unsigned long extid)
{
	const sbi_ext_t *ext;
	for (ext = __ext_start; ext < __ext_end; ext++) {
		if (ext->eid == extid) {
			return SBI_SUCCESS;
		}
	}

	return SBI_ERR_NOT_SUPPORTED;
}


static sbiret_t ecall_base_handler(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid)
{
	(void)a1;
	(void)a2;
	(void)a3;
	(void)a4;
	(void)a5;

	sbiret_t ret = {
		.error = SBI_SUCCESS,
	};

	switch (fid) {
		case BASE_GET_SPEC_VERSION:
			ret.value = SBI_VERSION(SBI_VERSION_MAJOR, SBI_VERSION_MINOR);
			break;

		case BASE_GET_IMPL_ID:
			ret.value = PHOENIX_SBI_ID;
			break;

		case BASE_GET_IMPL_VERSION:
			ret.value = PHOENIX_SBI_VERSION;
			break;

		case BASE_PROBE_EXT:
			ret.value = ecall_base_probeExt(a0);
			break;

		case BASE_GET_MVENDORID:
			ret.value = csr_read(CSR_MVENDORID);
			break;

		case BASE_GET_MARCHID:
			ret.value = csr_read(CSR_MARCHID);
			break;

		case BASE_GET_MIMPID:
			ret.value = csr_read(CSR_MIMPID);
			break;

		default:
			ret.error = SBI_ERR_NOT_SUPPORTED;
			ret.value = 0;
			break;
	}

	return ret;
}

static const sbi_ext_t sbi_ext_base __attribute__((section("extensions"), used)) = {
	.eid = SBI_EXT_BASE,
	.handler = ecall_base_handler,
};
