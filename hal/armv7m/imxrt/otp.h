/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * On-Chip OTP Controller interface i.MX RT series
 *
 * Copyright 2020-2022 Phoenix Systems
 * Author: Aleksander Kaminski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _OTP_H_
#define _OTP_H_


#define OTP_VERSION_MAJOR(v) (((v) >> 24) & 0xff)
#define OTP_VERSION_MINOR(v) (((v) >> 16) & 0xff)
#define OTP_VERSION_STEP(v)  ((v)&0xffff)


u32 otp_getVersion(void);


int otp_checkFuseValid(int fuse);


int otp_read(int fuse, u32 *val);


int otp_write(int fuse, u32 val);


void otp_init(void);


#endif /* _OTP_H_ */
