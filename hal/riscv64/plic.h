/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * RISCV64 PLIC interrupt controller driver
 *
 * Copyright 2020, 2024 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PLIC_H_
#define _PLIC_H_

#include <types.h>


void plic_priority(unsigned int n, unsigned int priority);


void plic_tresholdSet(unsigned int context, unsigned int priority);


unsigned int plic_claim(unsigned int context);


void plic_complete(unsigned int context, unsigned int n);


int plic_enableInterrupt(unsigned int context, unsigned int n);


int plic_disableInterrupt(unsigned int context, unsigned int n);


void _plic_init(void);


#endif
