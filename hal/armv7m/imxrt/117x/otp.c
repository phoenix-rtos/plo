/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * On-Chip OTP Controller interface i.MX RT117x
 *
 * Copyright 2020-2022 Phoenix Systems
 * Author: Aleksander Kaminski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>


#define FUSE_MIN   0x0800u
#define FUSE_MAX   0x18f0u
#define OCOTP_BASE ((void *)0x40cac000)


/* clang-format off */

enum { otp_ctrl = 0, otp_ctrl_set, otp_ctrl_clr, otp_ctrl_tog, otp_pdn,
	otp_data = 8, otp_read_ctrl = 12, otp_out_status = 36, otp_out_status_set, otp_out_status_clr,
	otp_out_status_tog, otp_version = 44, otp_read_fuse_data0 = 64, otp_read_fuse_data1 = 68,
	otp_read_fuse_data2 = 72, otp_read_fuse_data3 = 76, otp_sw_lock = 80, otp_bit_lock = 84,
	otp_locked0 = 384, otp_locked1 = 388, otp_locked2 = 392, otp_locked3 = 396, otp_locked4 = 400,
	otp_fuse = 512 };

/* clang-format on */


static struct {
	volatile u32 *base;
} otp_common;


static void otp_clrError(void)
{
	*(otp_common.base + otp_ctrl_clr) = 1 << 11;
	*(otp_common.base + otp_out_status_clr) = ~0u;
}


static void otp_waitBusy(void)
{
	time_t start;

	while (*(otp_common.base + otp_ctrl) & (1 << 10)) {
		;
	}

	/* Wait some more (at least 2 us) */
	for (start = hal_timerGet(); hal_timerGet() - start > 0;) {
		;
	}
}


int otp_checkFuseValid(int fuse)
{
	if ((fuse >= FUSE_MIN) && (fuse <= FUSE_MAX) && ((fuse & 0xf) == 0)) {
		return EOK;
	}

	return -ERANGE;
}


static u32 fuse2addr(int fuse)
{
	return ((fuse - FUSE_MIN) >> 4) & 0x3ff;
}


u32 otp_getVersion(void)
{
	return *(otp_common.base + otp_version);
}


int otp_read(int fuse, u32 *val)
{
	unsigned int t;

	int res = otp_checkFuseValid(fuse);
	if (res != EOK) {
		return res;
	}

	otp_clrError();
	otp_waitBusy();

	/* Set fuse address */
	t = *(otp_common.base + otp_ctrl) & ~0x3ffu;
	*(otp_common.base + otp_ctrl) = t | fuse2addr(fuse);

	/* Start read */
	t = *(otp_common.base + otp_read_ctrl) & ~0x1fu;
	*(otp_common.base + otp_read_ctrl) = t | (3 << 1) | 1;

	otp_waitBusy();

	*val = *(otp_common.base + otp_read_fuse_data0);

	if (*(otp_common.base + otp_out_status) & (1 << 24)) {
		*(otp_common.base + otp_out_status_clr) |= 1 << 24;

		return -EIO;
	}

	return EOK;
}


int otp_write(int fuse, u32 val)
{
	unsigned int t;

	int res = otp_checkFuseValid(fuse);
	if (res != EOK) {
		return res;
	}

	otp_clrError();
	otp_waitBusy();

	/* Set fuse address and unlock write */
	t = *(otp_common.base + otp_ctrl) & ~0xffff03ffu;
	*(otp_common.base + otp_ctrl) = t | (0x3e77u << 16) | fuse2addr(fuse);

	/* Program the word */
	*(otp_common.base + otp_data) = val;

	otp_waitBusy();

	t = *(otp_common.base + otp_out_status);

	if (t & (1 << 12)) {
		res = -EIO;
	}

	if (t & (1u << 11)) {
		res = -EPERM;
	}

	if (res != 0) {
		*(otp_common.base + otp_out_status_clr) |= 0x3 << 11;
	}

	return res;
}


void otp_init(void)
{
	otp_common.base = OCOTP_BASE;
}
