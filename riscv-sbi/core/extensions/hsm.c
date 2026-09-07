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
#include "boot.h"
#include "config.h"
#include "csr.h"
#include "hart.h"

#include "extensions/hsm.h"
#include "extensions/ipi.h"


static struct {
	volatile u32 hartsStarted;
} hsm_common;


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

	sbi_ipiSend(hartid, NULL, NULL);

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

	csr_set(CSR_MIE, MIP_MSIP | MIP_MEIP);

	atomic_add32(&hsm_common.hartsStarted, 1);

	while (ATOMIC_READ(&data->state) != SBI_HSM_START_PENDING) {
		__WFI();
	}

	csr_write(CSR_MIE, mie);
}


void hsm_init(u32 hartid)
{
	sbi_perHartData_t *data;
	u32 hartCount;

	if (hartid == BOOT_HART_ID) {
		atomic_add32(&hsm_common.hartsStarted, 1);
		data = sbi_getPerHartData(hartid);
		ATOMIC_WRITE(&data->state, SBI_HSM_START_PENDING);
		hartCount = sbi_getHartCount();
		if (hartCount > MAX_HART_COUNT) {
			hartCount = MAX_HART_COUNT;
		}
		while (ATOMIC_READ(&hsm_common.hartsStarted) < hartCount) {
			/* A hart may have announced itself in .bootstate after the
			 * initial sweep in _start and might still be waiting in flash.
			 * We cannot make progress until it arrives anyway. */
			boot_release(hartid);
		}
	}
	else {
		hsm_hartWait(hartid);
	}
}
