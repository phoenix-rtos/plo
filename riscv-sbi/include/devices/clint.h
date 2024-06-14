/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * CLINT driver
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_DEVICES_CLINT_H_
#define _SBI_DEVICES_CLINT_H_


#include "peripherals.h"


typedef struct {
	sbi_reg_t reg;
} clint_info_t;


void clint_timerIrqHandler(void);


void clint_setTimecmp(u64 time);


u64 clint_getTime(void);


void clint_init(void);


#endif
