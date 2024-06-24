/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * MCXN94x basic peripherals control functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_MCXN94X_H_
#define _HAL_MCXN94X_H_

#include "hal/armv8m/mcx/types.h"
#include <phoenix/arch/armv8m/mcx/n94x/mcxn94x.h>


extern int _mcxn94x_portPinConfig(int pin, int mux, int options);


extern int _mcxn94x_sysconSetDevClk(int dev, unsigned int sel, unsigned int div, int enable);


extern int _mcxn94x_sysconDevReset(int dev, int state);


extern u64 _mcxn94x_sysconGray2Bin(u64 gray);


extern void _mcxn94x_init(void);

#endif
