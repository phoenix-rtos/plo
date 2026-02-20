/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * STM32 MCE driver
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _MCE_H_
#define _MCE_H_

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>

#include <board_config.h>

#define MCE_CIPHER_AES128  1UL
#define MCE_CIPHER_NOEKEON 2UL
#define MCE_CIPHER_AES256  3UL

#define MCE_MODE_STREAM 1UL
#define MCE_MODE_NBLOCK 2UL
#define MCE_MODE_FBLOCK 3UL

/* MCE regions */
typedef enum {
	mce_r1,
	mce_r2,
	mce_r3,
	mce_r4,
	mce_regcount,
} mce_reg_t;


/* Public MCE interface */


int mce_configureRegion(int dev, mce_reg_t reg, const dev_memcrypt_args_t *args);


/* Get encryption granularity.
 * Returns:
 * * < 0 on error
 * * 0 if memory is bit-writable
 * * x > 0 if memory must be written x bytes at a time
 */
int mce_getGranularity(int dev, mce_reg_t reg);


#endif
