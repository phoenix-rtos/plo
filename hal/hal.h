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

#include <config.h>
#include <lib/types.h>


/* Function initializes clocks, peripherals and basic controllers */
extern void hal_init(void);


/* Function resets basic controllers */
extern void hal_done(void);


/* Function returns information about CPU */
extern const char *hal_cpuInfo(void);


/* Function sets kernel's entry address */
extern void hal_setKernelEntry(addr_t addr);


/* Function translates virtual address into physical */
extern addr_t hal_vm2phym(addr_t addr);


/* Function starts kernel loaded into memory */
extern int hal_launch(void);


/* Function invalidates all data cache */
extern void hal_invalDCacheAll(void);


/* Function invalidates data cache region */
extern void hal_invalDCacheAddr(addr_t addr, size_t sz);


/* Function disables interrupts */
extern void hal_cli(void);


/* Function enables interrupts */
extern void hal_sti(void);


/* Function registers interrupts in controller */
extern int hal_irqinst(u16 irq, int (*isr)(u16, void *), void *data);


/* Function removes interrupts from controller */
extern int hal_irquninst(u16 irq);


/* Function initializes console which uses polling methods to transmit data */
extern void hal_consoleInit(void);


/* Function writes data to uart controller */
extern void hal_consolePrint(const char *s);


#endif
