/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * BSEC OTP control STM32N6 series
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include "stm32n6_regs.h"


#define BSEC_BASE      ((void *)0x56009000u)
#define FUSE_MIN       0u
#define FUSE_MID_MIN   128u
#define FUSE_UPPER_MIN 256u
#define FUSE_MAX       375u

#define OTPSR_BUSY      (1u)
#define OTPSR_INIT_DONE (1u << 1)
#define OTPSR_HIDEUP    (1u << 2)
#define OTPSR_OTPNVIR   (1u << 4)
#define OTPSR_OTPERR    (1u << 5)
#define OTPSR_OTPSEC    (1u << 6)
#define OTPSR_PROGFAIL  (1u << 16)
#define OTPSR_DISTURB   (1u << 17)
#define OTPSR_DEDF      (1u << 18)
#define OTPSR_SECF      (1u << 19)
#define OTPSR_PPLF      (1u << 20)
#define OTPSR_PPLMF     (1u << 21)
#define OTPSR_AMEF      (1u << 22)

#define OTPCR_ADDR   (0x1FF)
#define OTPCR_PROG   (1u << 13)
#define OTPCR_PPLOCK (1u << 14)


static struct {
	volatile u32 *bsec_base;
} otp_common;


static void otp_waitBusy(void)
{
	/* Wait until not busy */
	while (*(otp_common.bsec_base + bsec_otpsr) & OTPSR_BUSY) {
		;
	}
}


static int otp_checkReadError(void)
{
	u32 t = *(otp_common.bsec_base + bsec_otpsr);
	if ((t & OTPSR_OTPERR) == 0) {
		return EOK;
	}

	/* Documentation recommends retrying if any of the following errors occur */
	if ((t & (OTPSR_AMEF | OTPSR_DEDF | OTPSR_DISTURB)) != 0) {
		return -EAGAIN;
	}

	/* The remaining flags (SECF, PPLF, PPLMF) signal conditions that don't make the fuse unreadable */
	return EOK;
}


int otp_checkFuseValid(int fuse)
{
	u32 fuseMax = FUSE_MAX;
	if (*(otp_common.bsec_base + bsec_otpsr) & OTPSR_HIDEUP) {
		fuseMax = FUSE_UPPER_MIN - 1;
	}
	if ((fuse >= FUSE_MIN) && (fuse <= fuseMax)) {
		return EOK;
	}
	return -EINVAL;
}


int otp_read(int fuse, u32 *val)
{
	unsigned int t;

	int res = otp_checkFuseValid(fuse);
	if (res != EOK) {
		return res;
	}
	otp_waitBusy();

	/* Set fuse address */
	t = *(otp_common.bsec_base + bsec_otpcr) & ~(OTPCR_ADDR | OTPCR_PROG | OTPCR_PPLOCK);
	*(otp_common.bsec_base + bsec_otpcr) = t | fuse;

	otp_waitBusy();

	res = otp_checkReadError();
	if (res != EOK) {
		return res;
	}

	/* Read the reloaded fuse */
	*val = *(otp_common.bsec_base + bsec_fvr0 + fuse);

	return EOK;
}


/* Write value or apply program lock to the selected fuse */
static int otp_writeInternal(int fuse, u32 val, int doPPLock)
{
	int res;
	unsigned int t, pplock;

	/* Set the word to program */
	*(otp_common.bsec_base + bsec_wdr) = val;

	pplock = (doPPLock != 0) ? OTPCR_PPLOCK : 0;
	t = *(otp_common.bsec_base + bsec_otpcr) & ~(OTPCR_ADDR | OTPCR_PROG | OTPCR_PPLOCK);
	*(otp_common.bsec_base + bsec_otpcr) = t | fuse | OTPCR_PROG | pplock;

	otp_waitBusy();

	t = *(otp_common.bsec_base + bsec_otpsr);
	if ((t & OTPSR_PROGFAIL) != 0) {
		return -EAGAIN;
	}

	/* Reload the fuse word */
	t = *(otp_common.bsec_base + bsec_otpcr) & ~(OTPCR_ADDR | OTPCR_PROG | OTPCR_PPLOCK);
	*(otp_common.bsec_base + bsec_otpcr) = t | fuse;

	otp_waitBusy();

	res = otp_checkReadError();
	if (res != EOK) {
		return res;
	}

	if (doPPLock != 0) {
		/* Check if program lock was applied correctly */
		if ((*(otp_common.bsec_base + bsec_otpsr) & OTPSR_PPLF) != 0) {
			return -EAGAIN;
		}
	}
	else {
		/* Compare the loaded word to val */
		if (*(otp_common.bsec_base + bsec_fvr0 + fuse) != val) {
			return -EAGAIN;
		}
	}

	return EOK;
}


int otp_write(int fuse, u32 val)
{
	int res = otp_checkFuseValid(fuse);
	if (res != EOK) {
		return res;
	}

	otp_waitBusy();
	res = otp_writeInternal(fuse, val, 0);
	if (res != EOK) {
		return res;
	}

	/* Middle and upper fuses should only be programmed once according to documentation */
	if (fuse >= FUSE_MID_MIN) {
		/* BSEC_WDR should be cleared when applying program lock, so set val to 0 */
		res = otp_writeInternal(fuse, 0, 1);
	}

	return res;
}


void otp_init(void)
{
	u32 t;
	otp_common.bsec_base = BSEC_BASE;

	/* Wait until not busy and BSEC initialized */
	do {
		t = *(otp_common.bsec_base + bsec_otpsr);
	} while (((t & OTPSR_BUSY) != 0) || ((t & OTPSR_INIT_DONE) == 0));
}
