/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * CSR functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "csr.h"
#include "types.h"

#include "devices/clint.h"


int csr_emulateRead(u32 csr, u64 *val)
{
	int ret = 0;

	switch (csr) {
		case CSR_TIME:
			*val = clint_getTime();
			break;

		case CSR_INSTRET:
			*val = csr_read(CSR_MINSTRET);
			break;

		case CSR_CYCLE:
			*val = csr_read(CSR_MCYCLE);
			break;

		default:
			ret = -1;
			break;
	}

	return ret;
}


int csr_emulateWrite(u32 csr, u64 val)
{
	(void)csr;
	(void)val;

	/* Applicable only for H-mode */

	return -1;
}
