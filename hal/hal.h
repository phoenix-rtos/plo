/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Hardware Abstraction Layer
 *
 * Copyright 2012, 2017, 2020-2021 Phoenix Systems
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


enum { hal_cpuICache = 0, hal_cpuDCache };


/* Function initializes clocks, peripherals and basic controllers */
extern void hal_init(void);

/* Function resets basic controllers */
extern void hal_done(void);


/* Function returns information about CPU */
extern const char *hal_cpuInfo(void);

/* Function invalidates data or instruction cache */
extern void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz);

/* Function invalidates the whole data or instruction cache */
extern void hal_cpuInvCacheAll(unsigned int type);


/* Function translates virtual address into physical */
extern addr_t hal_kernelGetAddress(addr_t addr);

/* Function starts kernel loaded into memory */
extern int hal_cpuJump(addr_t addr);


/* Function enables interrupts */
extern void hal_interruptsEnable(void);

/* Function disables interrupts */
extern void hal_interruptsDisable(void);

/* Function registers interrupts in controller */
extern int hal_interruptsSet(unsigned int irq, int (*isr)(unsigned int, void *), void *data);


/* Function returns time in milliseconds from plo start */
extern time_t hal_timerGet(void);


/* Function writes data to uart controller */
extern void hal_consolePrint(const char *s);


#endif
