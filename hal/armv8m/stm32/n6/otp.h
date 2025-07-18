/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * BSEC OTP control STM32N6 series
 *
 * Copyright 2020-2022 2025 Phoenix Systems
 * Author: Aleksander Kaminski, Gerard Swiderski, Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <../types.h>

#ifndef _OTP_H_
#define _OTP_H_

// const char *otp_errorStr(int err);


int otp_checkFuseValid(int fuse);


int otp_read(int fuse, u32 *val);


int otp_write(int fuse, u32 val);


void otp_init(void);


#endif /* _OTP_H_ */
