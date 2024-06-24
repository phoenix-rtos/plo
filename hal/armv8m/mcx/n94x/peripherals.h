/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Peripherals definitions for armv8m33-mcxn94x
 *
 * Copyright 2024 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include <board_config.h>

/* Periperals configuration */

#define SIZE_INTERRUPTS (171 + 16)

#define FLASH_PROGRAM_1_ADDR    0x00000000u
#define FLASH_PROGRAM_2_ADDR    0x00100000u
#define FLASH_PROGRAM_BANK_SIZE (1024 * 1024)

#endif
