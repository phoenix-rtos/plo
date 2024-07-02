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
#include "extensions/ipi.h"


#define SFENCE_VMA_FLUSH_ALL (size_t)(-1)


enum {
	RFENCE_FENCE_I = 0,
	RFENCE_SFENCE_VMA,
	RFENCE_SFENCE_VMA_ASID,
	RFENCE_HFENCE_GVMA_VMID,
	RFENCE_HFENCE_GVMA,
	RFENCE_HFENCE_VVMA_ASID,
	RFENCE_HFENCE_VVMA
};


typedef struct {
	addr_t start;
	size_t size;
	u32 asid;
} sfenceVmaInfo_t;


static void ecall_rfence_fenceiHandler(void *data)
{
	(void)data;

	__asm__ volatile("fence.i");
}


static void ecall_rfence_sfenceVmaHandler(void *data)
{
	size_t i;
	sfenceVmaInfo_t *info = (sfenceVmaInfo_t *)data;

	if (((info->start == 0) && (info->size == 0)) || (info->size == SFENCE_VMA_FLUSH_ALL)) {
		__asm__ volatile("sfence.vma" ::: "memory");

		return;
	}

	for (i = 0; i < info->size; i += PAGE_SIZE) {
		/* clang-format off */
		__asm__ volatile (
			"sfence.vma %0"
			:
			: "r"(info->start + i)
			: "memory"
		);
		/* clang-format on */
	}
}


static void ecall_rfence_sfenceVmaAsidHandler(void *data)
{
	size_t i;
	sfenceVmaInfo_t *info = (sfenceVmaInfo_t *)data;

	if (((info->start == 0) && (info->size == 0)) || (info->size == SFENCE_VMA_FLUSH_ALL)) {
		/* clang-format off */
		__asm__ volatile (
			"sfence.vma x0, %0"
			:
			: "r"(info->asid)
			: "memory"
		);
		/* clang-format on */

		return;
	}


	for (i = 0; i < info->size; i += PAGE_SIZE) {
		/* clang-format off */
		__asm__ volatile (
			"sfence.vma %0, %1"
			:
			: "r"(info->start + i), "r"(info->asid)
			: "memory"
		);
		/* clang-format on */
	}
}


static sbiret_t ecall_rfence_handler(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid)
{
	sbiret_t ret = { 0 };
	sfenceVmaInfo_t info;

	switch (fid) {
		case RFENCE_FENCE_I:
			ret.error = sbi_ipiSendMany(a0, a1, ecall_rfence_fenceiHandler, NULL);
			break;

		case RFENCE_SFENCE_VMA:
			info.start = a2;
			info.size = a3;
			ret.error = sbi_ipiSendMany(a0, a1, ecall_rfence_sfenceVmaHandler, &info);
			break;

		case RFENCE_SFENCE_VMA_ASID:
			info.start = a2;
			info.size = a3;
			info.asid = a4;
			ret.error = sbi_ipiSendMany(a0, a1, ecall_rfence_sfenceVmaAsidHandler, &info);
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
