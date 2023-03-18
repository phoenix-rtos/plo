/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Peripherals definitions for Leon3 GR716
 *
 * Copyright 2022 Phoenix Systems
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

#define GRGPIO0_BASE ((void *)0x8030C000)
#define GRGPIO1_BASE ((void *)0x8030D000)

/* Interrupts */

#define INT_CTRL_BASE ((void *)0x80002000)

/* Timers */

#define TIMER_IRQ     9
#define GPTIMER0_BASE ((void *)0x80003000)
#define GPTIMER1_BASE ((void *)0x80004000)

/* SPI Flash */

#define SPIMCTRL0_BASE ((void *)0xFFF00100)
#define SPIMCTRL1_BASE ((void *)0xFFF00200)


#endif
