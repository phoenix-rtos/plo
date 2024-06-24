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

#define FC0_BASE ((void *)0x40092000)
#define FC1_BASE ((void *)0x40093000)
#define FC2_BASE ((void *)0x40094000)
#define FC3_BASE ((void *)0x40095000)
#define FC4_BASE ((void *)0x400b4000)
#define FC5_BASE ((void *)0x400b5000)
#define FC6_BASE ((void *)0x400b6000)
#define FC7_BASE ((void *)0x400b7000)
#define FC8_BASE ((void *)0x400b8000)
#define FC9_BASE ((void *)0x400b9000)

#define FC0_IRQ (35 + 16)
#define FC1_IRQ (36 + 16)
#define FC2_IRQ (37 + 16)
#define FC3_IRQ (38 + 16)
#define FC4_IRQ (39 + 16)
#define FC5_IRQ (40 + 16)
#define FC6_IRQ (41 + 16)
#define FC7_IRQ (42 + 16)
#define FC8_IRQ (43 + 16)
#define FC9_IRQ (44 + 16)

#define UART_CLK      12000000 /* FRO_12M */
#define UART_BAUDRATE 115200

#endif