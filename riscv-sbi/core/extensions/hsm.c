/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI HSM extension
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "atomic.h"
#include "hart.h"
#include "csr.h"

#include "extensions/hsm.h"


extern u32 bootHartId;


long hsm_hartStart(sbi_param hartid, sbi_param startAddr, sbi_param opaque)
{
	sbi_perHartData_t *data;
	int state;

	if (hartid >= sbi_getHartCount()) {
		return SBI_ERR_INVALID_PARAM;
	}

	data = sbi_getPerHartData(hartid);

	state = atomic_cas64(&data->state, SBI_HSM_STOPPED, SBI_HSM_START_PENDING);
	if (state == SBI_HSM_STARTED) {
		return SBI_ERR_ALREADY_AVAILABLE;
	}

	if (state != SBI_HSM_STOPPED) {
		return SBI_ERR_INVALID_PARAM;
	}

	ATOMIC_WRITE(&data->nextAddr, startAddr);
	ATOMIC_WRITE(&data->nextArg1, opaque);

	return SBI_SUCCESS;
}


sbiret_t hsm_hartGetStatus(sbi_param hartid)
{
	sbi_perHartData_t *data;

	if (hartid >= sbi_getHartCount()) {
		return (sbiret_t) { .error = SBI_ERR_INVALID_PARAM };
	}

	data = sbi_getPerHartData(hartid);

	return (sbiret_t) { .error = SBI_SUCCESS, .value = ATOMIC_READ(&data->state) };
}


void __attribute__((noreturn)) hsm_hartStartJump(u32 hartid)
{
	sbi_perHartData_t *data = sbi_getPerHartData(hartid);

	if (atomic_cas64(&data->state, SBI_HSM_START_PENDING, SBI_HSM_STARTED) != SBI_HSM_START_PENDING) {
		/* This should never happen */
		hart_halt();
	}

	hart_changeMode(hartid, ATOMIC_READ(&data->nextArg1), ATOMIC_READ(&data->nextAddr), PRV_S);
}


static void hsm_hartWait(u32 hartid)
{
	sbi_perHartData_t *data = sbi_getPerHartData(hartid);
	unsigned long mie = csr_read(CSR_MIE);

	csr_clear(CSR_MIE, MIP_MSIP | MIP_MEIP);

	while (ATOMIC_READ(&data->state) != SBI_HSM_START_PENDING) {
		__WFI();
	}

	csr_write(CSR_MIE, mie);
}


void hsm_init(u32 hartid)
{
	sbi_perHartData_t *data;

	if (hartid == bootHartId) {
		data = sbi_getPerHartData(hartid);
		ATOMIC_WRITE(&data->state, SBI_HSM_START_PENDING);
	}
	else {
		hsm_hartWait(hartid);
	}
}
