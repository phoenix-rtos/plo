/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2012, 2017, 2020 Phoenix Systems
 * Copyright 2006 Radoslaw F. Wawrzusiak
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Authors: Pawel Pisarczyk, Radoslaw F. Wawrzusiak, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_H_
#define _HAL_H_

#include "config.h"
#include "string.h"
#include "types.h"
#include "peripherals.h"


/* Initialization functions */

extern void hal_init(void);

extern void hal_done(void);


/* Setters and getters for common data */

extern void hal_setDefaultIMAP(char *imap);

extern void hal_setDefaultDMAP(char *dmap);

extern void hal_setKernelEntry(addr_t addr);

extern addr_t hal_vm2phym(addr_t addr);


/* Function starts kernel loaded into memory */

extern int hal_launch(void);


/* Cache */

extern void hal_invalDCacheAll(void);

extern void hal_invalDCacheAddr(addr_t addr, size_t sz);


/* Opeartions on interrupts */

extern void hal_cli(void);

extern void hal_sti(void);

extern int hal_irqinst(u16 irq, int (*isr)(u16, void *), void *data);

extern int hal_irquninst(u16 irq);


/* Console functions */

extern void hal_consoleInit(void);

extern void hal_consolePrint(const char *s);


#endif
