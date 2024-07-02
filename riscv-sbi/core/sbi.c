/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "csr.h"
#include "fdt.h"
#include "hart.h"
#include "sbi.h"

#include "devices/clint.h"
#include "devices/console.h"

#include "extensions/hsm.h"
#include "extensions/ipi.h"

#include "ld/noelv.ldt"


extern const void *__payload_start;

extern const sbi_ext_t __ext_start[];
extern const sbi_ext_t __ext_end[];


volatile u64 sbi_hartMask;


static struct {
	volatile u32 hartCount;
	sbi_perHartData_t perHartData[MAX_HART_COUNT] __attribute__((aligned(8)));
} sbi_common;


unsigned int sbi_getFirstBit(unsigned long v)
{
	unsigned int fb = 0;

	if (!(v & 0xffffffffL)) {
		fb += 32;
		v = (v >> 32);
	}

	if (!(v & 0xffff)) {
		fb += 16;
		v = (v >> 16);
	}

	if (!(v & 0xff)) {
		fb += 8;
		v = (v >> 8);
	}

	if (!(v & 0xf)) {
		fb += 4;
		v = (v >> 4);
	}

	if (!(v & 0x3)) {
		fb += 2;
		v = (v >> 2);
	}

	if (!(v & 0x01))
		fb += 1;

	return fb;
}


sbiret_t sbi_dispatchEcall(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid, int eid)
{
	const sbi_ext_t *ext;

	sbiret_t ret = {
		.error = SBI_ERR_NOT_SUPPORTED,
		.value = 0
	};

	for (ext = __ext_start; ext < __ext_end; ext++) {
		if (ext->eid == eid) {
			ret = ext->handler(a0, a1, a2, a3, a4, a5, fid);
			break;
		}
	}

	return ret;
}


sbi_perHartData_t *sbi_getPerHartData(u32 hartid)
{
	return &sbi_common.perHartData[hartid];
}


u32 sbi_getHartCount(void)
{
	return sbi_common.hartCount;
}


void sbi_scratchInit(u32 hartid, const void *fdt, addr_t sp)
{
	sbi_perHartData_t *data = &sbi_common.perHartData[hartid];
	data->mstack = sp;
	data->state = SBI_HSM_STOPPED;
	data->nextAddr = (addr_t)&__payload_start;
	data->nextArg1 = (addr_t)fdt;
	RISCV_FENCE(w, rw);
	csr_write(CSR_MSCRATCH, data);
}


void __attribute__((noreturn)) sbi_initCold(u32 hartid, const void *fdt)
{
	fdt_init(fdt);

	sbi_common.hartCount = fdt_parseCpus();

	hsm_init(hartid);

	hart_init();
	console_init();
	clint_init();
	sbi_ipiInit();

	console_print("Phoenix SBI\n");

	hsm_hartStartJump(hartid);
}


void __attribute__((noreturn)) sbi_initWarm(u32 hartid)
{
	hsm_init(hartid);

	hart_init();

	hsm_hartStartJump(hartid);
}
