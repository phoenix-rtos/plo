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

#include <board_config.h>
#include <hal/armv8m/stm32/n6/stm32n6.h>

#define MCE_CIPHER_AES128  1UL
#define MCE_CIPHER_NOEKEON 2UL
#define MCE_CIPHER_AES256  3UL

#define MCE_MODE_STREAM 1UL
#define MCE_MODE_NBLOCK 2UL
#define MCE_MODE_FBLOCK 3UL

/* MCE peripherals */
typedef enum {
	mce1,
	mce2,
	mce3,
	mce4,
	mce_count,
} mce_t;

/* MCE regions */
typedef enum {
	mce_r1,
	mce_r2,
	mce_r3,
	mce_r4,
	mce_regcount,
} mce_reg_t;


/* Public MCE interface */


int mce_configureRegion(mce_t per, mce_reg_t reg, addr_t start, addr_t end, u32 cipher, u32 mode, u8 *key);


void mce_disable(mce_t per, mce_reg_t reg);


#endif
