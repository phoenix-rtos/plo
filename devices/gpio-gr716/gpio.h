/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 GPIO driver
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#include <hal/hal.h>

#define GPIO_PORT_0 0
#define GPIO_PORT_1 1

#define GPIO_DIR_IN  0
#define GPIO_DIR_OUT 1


extern int gpio_writePin(u8 pin, u8 val);


extern int gpio_readPin(u8 pin, u8 *val);


extern int gpio_getPinDir(u8 pin, u8 *dir);


extern int gpio_setPinDir(u8 pin, u32 dir);


extern void gpio_init(void);


#endif
