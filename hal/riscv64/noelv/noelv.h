/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * NOEL-V CPU specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _NOELV_H_
#define _NOELV_H_


#include "types.h"


typedef struct {
	u8 pin;
	u8 opt;
	u8 pullup;
	u8 pulldn;
} iomux_cfg_t;


static inline int gaisler_iomuxCfg(iomux_cfg_t *ioCfg)
{
	(void)ioCfg;

	return 0;
}


void noelv_init(void);


#endif
