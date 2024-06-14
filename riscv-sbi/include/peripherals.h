/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * Peripherals
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_PERIPHERALS_H_
#define _SBI_PERIPHERALS_H_


#include "types.h"


typedef struct {
	addr_t base;
	size_t size;
} sbi_reg_t;


#endif
