/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GPIO controller
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#include <hal/hal.h>

enum { gpio_dir_in = 0, gpio_dir_out = 1 };


/* The Zynq 7000 has access to:
 *  - MIO pins id [31:0],
 *  - MIO pins id [53:32],
 *  - EMIO signals id [85:54],
 *  - EMIO signals id [117:86] */
extern int gpio_writePin(u8 pin, u8 val);

extern int gpio_readPin(u8 pin, u8 *val);

extern int gpio_getPinDir(u8 pin, u8 *dir);

extern int gpio_setPinDir(u8 pin, u8 dir);


/* The Zynq 7000 has access to:
 * - Bank0: id = 0,
 * - Bank1: id = 1,
 * - Bank2: id = 2,
 * - Bank3: id = 3  */
extern int gpio_writeBank(u8 bank, u32 val);

extern int gpio_readBank(u8 bank, u32 *val);

extern int gpio_getBankDir(u8 bank, u32 *dir);

extern int gpio_setBankDir(u8 bank, u32 dir);



extern void gpio_init(void);

extern void gpio_deinit(void);


#endif
