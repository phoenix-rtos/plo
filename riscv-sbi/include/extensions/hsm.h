/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI HSM extension
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_EXT_HSM_H_
#define _SBI_EXT_HSM_H_


#include "sbi.h"
#include "types.h"


long hsm_hartStart(sbi_param hartid, sbi_param startAddr, sbi_param opaque);


sbiret_t hsm_hartGetStatus(sbi_param hartid);


void __attribute__((noreturn)) hsm_hartStartJump(u32 hartid);


void hsm_init(u32 hartid);


#endif
