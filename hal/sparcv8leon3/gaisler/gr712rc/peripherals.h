/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Peripherals definitions for Leon3 GR712RC
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include <board_config.h>

/* Timer registers */

#define GPT_SCALER  0 /* Scaler value register        : 0x00 */
#define GPT_SRELOAD 1 /* Scaler reload value register : 0x04 */
#define GPT_CONFIG  2 /* Configuration register       : 0x08 */

#define GPT_TCNTVAL1 4 /* Timer 1 counter value reg    : 0x10 */
#define GPT_TRLDVAL1 5 /* Timer 1 reload value reg     : 0x14 */
#define GPT_TCTRL1   6 /* Timer 1 control register     : 0x18 */

#define GPT_TCNTVAL2 8  /* Timer 2 counter value reg    : 0x20 */
#define GPT_TRLDVAL2 9  /* Timer 2 reload value reg     : 0x24 */
#define GPT_TCTRL2   10 /* Timer 2 control register     : 0x28 */

#define GPT_TCNTVAL3 12 /* Timer 3 counter value reg    : 0x30 */
#define GPT_TRLDVAL3 13 /* Timer 3 reload value reg     : 0x34 */
#define GPT_TCTRL3   14 /* Timer 3 control register     : 0x38 */

#define GPT_TCNTVAL4 16 /* Timer 4 counter value reg    : 0x40 */
#define GPT_TRLDVAL4 17 /* Timer 4 reload value reg     : 0x44 */
#define GPT_TCTRL4   18 /* Timer 4 control register     : 0x48 */

/* Arbitrarily chosen address */
#define RAM_ADDR      (0x47e00000)
#define RAM_BANK_SIZE (0x200000)

#endif
