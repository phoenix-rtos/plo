/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Gaisler CPU specific functions
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _GAISLER_H_
#define _GAISLER_H_


#include "types.h"


typedef struct {
	u8 pin;
	u8 opt;
	u8 pullup;
	u8 pulldn;
} iomux_cfg_t;


int gaisler_iomuxCfg(iomux_cfg_t *ioCfg);


#endif
