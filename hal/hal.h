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
#include "string.h"


enum { hal_cpuICache = 0, hal_cpuDCache };


/* Function initializes clocks, peripherals and basic controllers */
extern void hal_init(void);


/* Function resets basic controllers */
extern void hal_done(void);


extern void hal_syspageSet(hal_syspage_t *hs);


/* Function returns information about CPU */
extern const char *hal_cpuInfo(void);


/* Function invalidates data or instruction cache */
extern void hal_cpuInvCache(unsigned int type, addr_t addr, size_t sz);


/* Function reboots system (final state may depend on latched bootloader config) */
extern void hal_cpuReboot(void) __attribute__((noreturn));


/* Function starts kernel loaded into memory */
extern int hal_cpuJump(void);


/* Function translates virtual address into physical */
extern addr_t hal_kernelGetAddress(addr_t addr);


extern void hal_kernelEntryPoint(addr_t addr);


extern void hal_kernelGetEntryPointOffset(addr_t *off, int *indirect);


/* Function validates and add memory map at hal region level */
extern int hal_memoryAddMap(addr_t start, addr_t end, u32 attr, u32 mapId);


/* Function returns entry located near the start of the declared memory */
extern int hal_memoryGetNextEntry(addr_t start, addr_t end, mapent_t *entry);


/* Functions enables/disable all interrupts */
extern void hal_interruptsEnableAll(void);


extern void hal_interruptsDisableAll(void);


/* Functions enables/disable specific interrupt */

extern void hal_interruptsEnable(unsigned int irqn);


extern void hal_interruptsDisable(unsigned int irqn);


/* Function registers interrupts in controller */
extern int hal_interruptsSet(unsigned int irq, int (*isr)(unsigned int, void *), void *data);


/* Function returns time in milliseconds from plo start */
extern time_t hal_timerGet(void);


/* Function sets early console hooks */
extern void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t));


/* Function writes data to uart controller */
extern void hal_consolePrint(const char *s);


#endif
