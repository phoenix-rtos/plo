/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * BSEC OTP control STM32N6 series
 *
 * Copyright 2025 Phoenix Systems
 * Author: Aleksander Kaminski, Gerard Swiderski, Krzysztof Radzewicz
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

#define OTPCR_ADDR   (0x1ff)
#define OTPCR_PROG   (1u << 13)
#define OTPCR_PPLOCK (1u << 14)


static struct {
	volatile u32 *bsec_base;
} otp_common;



static void otp_waitBusy(void)
{
	/* Wait untill not busy */
	while (*(otp_common.bsec_base + bsec_otpsr) & OTPSR_BUSY) {
		;
	}
}


static int otp_checkError(void)
{
	u32 t = *(otp_common.bsec_base + bsec_otpsr);
	if ((t & OTPSR_OTPERR) == 0) {
		return EOK;
	}
	return t & 0x7F0077;
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
	return -ERANGE;
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

	res = otp_checkError();
	if (res != EOK) {
		return res;
	}

	/* Read the reloaded fuse */
	*val = *(otp_common.bsec_base + bsec_fvr0 + fuse);

	return EOK;
}


int otp_write(int fuse, u32 val)
{
	unsigned int t, lockFuse = 0;

	int res = otp_checkFuseValid(fuse);
	if (res != EOK) {
		return res;
	}

	otp_waitBusy();

	/* Set the word to program */
	*(otp_common.bsec_base + bsec_wdr) = val;

	if (fuse >= FUSE_MID_MIN) {
		lockFuse = ~0u;
	}

	/* Program the word using cr register. Fuse word is locked if it's mid or upper */
	t = *(otp_common.bsec_base + bsec_otpcr) & ~(OTPCR_ADDR | OTPCR_PROG | OTPCR_PPLOCK);
	*(otp_common.bsec_base + bsec_otpcr) |= fuse | OTPCR_PROG | (lockFuse & OTPCR_PPLOCK);

	otp_waitBusy();

	t = *(otp_common.bsec_base + bsec_otpsr);
	if (t & OTPSR_PROGFAIL) {
		return -EAGAIN;
	}
	if (t & OTPSR_PPLF) {
		return -EPERM;
	}
	if (t & OTPSR_PPLMF) {
		return -EINVAL;
	}

	/* Reload the fuse word */
	t = *(otp_common.bsec_base + bsec_otpcr) & ~(OTPCR_ADDR | OTPCR_PROG | OTPCR_PPLOCK);
	*(otp_common.bsec_base + bsec_otpcr) = t | fuse;

	otp_waitBusy();

	if (*(otp_common.bsec_base + bsec_otpsr) & OTPSR_OTPERR) {
		return -EAGAIN;
	}
	/* Compare the loaded word to val*/
	if (*(otp_common.bsec_base + bsec_fvr0 + fuse) != val) {
		return -EAGAIN;
	}

	return EOK;
}


void otp_init(void)
{
	u32 t;
	otp_common.bsec_base = BSEC_BASE;

	/* Wait untill not busy and BSEC initialized */
	while (((t = *(otp_common.bsec_base + bsec_otpsr)) & OTPSR_BUSY) || ((t & OTPSR_INIT_DONE) == 0)) {
		;
	}
}
