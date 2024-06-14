/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * CLINT driver
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
#include "types.h"

#include "devices/clint.h"


#define CLINT_MSIP(hartid)     (0x0 + 4 * (hartid))
#define CLINT_MTIMECMP(hartid) (0x4000 + 8 * (hartid))
#define CLINT_MTIMER           0xbff8


static struct {
	addr_t base;
} clint_common;


void clint_timerIrqHandler(void)
{
	/* Disable MTIMER interrupt */
	csr_clear(CSR_MIE, MIP_MTIP);

	/* Set STIMER interrupt pending */
	csr_set(CSR_MIP, MIP_STIP);
}


void clint_setTimecmp(u64 time)
{
	u32 hartid = csr_read(CSR_MHARTID);

	*((vu64 *)(clint_common.base + CLINT_MTIMECMP(hartid))) = time;

	csr_clear(CSR_MIP, MIP_STIP);

	csr_set(CSR_MIE, MIP_MTIP);
}


u64 clint_getTime(void)
{
	return *((vu64 *)(clint_common.base + CLINT_MTIMER));
}


void clint_init(void)
{
	clint_info_t info;

	if (fdt_getClintInfo(&info) < 0) {
		return;
	}
	clint_common.base = info.reg.base;
}
