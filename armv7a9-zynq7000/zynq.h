/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * zynq-7000 basic peripherals control functions
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _ZYNQ_H_
#define _ZYNQ_H_


#include "cpu.h"

#include "../errors.h"
#include "../types.h"


/* Function loads bitstream data from specific memory address */
extern int _zynq_loadPL(u32 srcAddr, u32 srcLen);


/* Function initializes plls, clocks, ddr and basic peripherals */
extern void _zynq_init(void);


#endif