/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Peripherals definitions for Leon GR716
 *
 * Copyright 2022-2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include <board_config.h>

/* GPIO */

#define GRGPIO0_BASE ((void *)0x80000900)
#define GRGPIO1_BASE ((void *)0x80000a00)

/* SPI Flash */

#define SPIMCTRL0_BASE ((void *)0xfff00100)
#define SPIMCTRL1_BASE ((void *)0xfff00200)


#endif
