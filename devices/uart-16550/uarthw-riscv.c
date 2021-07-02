/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * UART 16550 (RISC-V)
 *
 * Copyright 2020, 2021 Phoenix Systems
 * Author: Julia Kosowska, Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>

#include "uart-16550.h"


typedef struct {
	void *base;
	unsigned int irq;
} uarthw_context_t;


static const uarthw_context_t uarts[] = {
	{ (void *)0x10000000, 10 }
};


unsigned char uarthw_read(void *hwctx, unsigned int reg)
{
	unsigned int addr = (unsigned int)((uarthw_context_t *)hwctx)->base;

	return *(volatile unsigned char *)(addr + reg);
}


void uarthw_write(void *hwctx, unsigned int reg, unsigned char val)
{
	unsigned int addr = (unsigned int)((uarthw_context_t *)hwctx)->base;

	*(volatile unsigned char *)(addr + reg) = val;
}


unsigned int uarthw_irq(void *hwctx)
{
	return ((uarthw_context_t *)hwctx)->irq;
}


int uarthw_init(unsigned int n, void *hwctx, unsigned int *baud)
{
	if (n >= sizeof(uarts) / sizeof(uarts[0]))
		return -ENOENT;

	/* Set UART hardware context */
	*(uarthw_context_t *)hwctx = uarts[n];

	/* Set preferred baudrate */
	if (baud != NULL)
		*baud = BPS_115200;

	return EOK;
}
