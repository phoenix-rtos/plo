/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * IPI definitions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _SBI_IPI_H_
#define _SBI_IPI_H_


#include "types.h"


typedef struct {
	void (*handler)(void *);
	void *data;
} ipi_task_t;


void sbi_ipiHandler(void);


long sbi_ipiSend(u32 hartid, void (*handler)(void *), void *data);


long sbi_ipiSendMany(unsigned long hartMask, unsigned long hartMaskBase, void (*handler)(void *), void *data);


void sbi_ipiInit(void);


#endif
