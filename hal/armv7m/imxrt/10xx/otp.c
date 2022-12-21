/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * On-Chip OTP Controller interface i.MX RT10xx
 *
 * Copyright 2021 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>


#define FUSE_MIN1 0x400u
#define FUSE_MAX1 0x6f0u
#define FUSE_MIN2 0x800u
#define FUSE_MAX2 0x8f0u

#define OCOTP_BASE ((void *)0x401f4000)


/* clang-format off */

enum { otp_ctrl = 0, otp_ctrl_set, otp_ctrl_clr, otp_ctrl_tog, otp_timing,
	otp_data = 8, otp_read_ctrl = 12, otp_read_fuse_data = 16, otp_sw_sticky = 20,
	otp_scs = 24, otp_scs_set, otp_scs_clr, otp_scs_tog, otp_crc_addr, otp_crc_value = 32,
	otp_version = 36, otp_timing2 = 64 };

/* clang-format on */


static struct {
	volatile u32 *base;
} otp_common;


static void otp_waitBusy(void)
{
	time_t start;

	while (*(otp_common.base + otp_ctrl) & (1 << 8)) {
		;
	}

	/* Wait some more (at least 2 us) */
	for (start = hal_timerGet(); hal_timerGet() - start > 0;) {
		;
	}
}


int otp_checkFuseValid(int fuse)
{
	if ((fuse & 0xf) == 0) {
		if (((fuse >= FUSE_MIN1) && (fuse <= FUSE_MAX1)) || ((fuse >= FUSE_MIN2) && (fuse < FUSE_MAX2))) {
			return EOK;
		}
	}

	return -ERANGE;
}


static u32 fuse2addr(int fuse)
{
	if (fuse >= FUSE_MIN2) {
		fuse -= 0x100;
	}

	return ((fuse - FUSE_MIN1) >> 4) & 0x3f;
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

	/* Clear error flag */
	*(otp_common.base + otp_ctrl_clr) |= 1 << 9;

	otp_waitBusy();

	/* Set fuse address */
	t = *(otp_common.base + otp_ctrl) & ~0x3fu;
	*(otp_common.base + otp_ctrl) = t | fuse2addr(fuse);

	/* Start read */
	t = *(otp_common.base + otp_read_ctrl) & ~1u;
	*(otp_common.base + otp_read_ctrl) = t | 1;

	otp_waitBusy();

	*val = *(otp_common.base + otp_read_fuse_data);

	if (*(otp_common.base + otp_ctrl) & (1 << 9)) {
		*(otp_common.base + otp_ctrl_clr) |= 1 << 9;
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

	/* Clear error flag */
	*(otp_common.base + otp_ctrl_clr) = 1 << 9;

	otp_waitBusy();

	/* Set fuse address and unlock write */
	t = *(otp_common.base + otp_ctrl) & ~0xffff003fu;
	*(otp_common.base + otp_ctrl) = t | (0x3e77u << 16) | fuse2addr(fuse);

	/* Program word */
	*(otp_common.base + otp_data) = val;

	otp_waitBusy();

	if (*(otp_common.base + otp_ctrl) & (1 << 9)) {
		*(otp_common.base + otp_ctrl_clr) |= 1 << 9;
		return -EIO;
	}

	return res;
}


void otp_init(void)
{
	otp_common.base = OCOTP_BASE;
}
