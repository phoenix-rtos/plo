/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Syspage
 *
 * Copyright 2020-2021 Phoenix Systems
 * Authors: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef SYSPAGE_H_
#define SYSPAGE_H_

#include <hal/hal.h>


enum { flagSyspageExec = 0x01, flagSyspageNoCopy = 0x02 };


/* General functions */
extern void syspage_init(void);


extern void *syspage_alloc(size_t size);


extern void syspage_kernelPAddrAdd(addr_t address);


/* Map's functions */
extern int syspage_mapAdd(const char *name, addr_t start, addr_t end, const char *attr);


extern int syspage_mapAttrResolve(const char *name, unsigned int *attr);


extern int syspage_mapNameResolve(const char *name, u8 *id);


extern int syspage_mapRangeResolve(const char *name, addr_t *start, addr_t *end);


extern unsigned int syspage_mapRangeCheck(addr_t start, addr_t end, unsigned int *attrOut);


extern const char *syspage_mapName(u8 id);


extern void syspage_mapShow(void);


extern mapent_t *syspage_entryAdd(const char *mapName, addr_t start, size_t size, unsigned int align);


/* Program's functions */
extern syspage_prog_t *syspage_progAdd(const char *argv, u32 flags);


extern void syspage_progShow(void);


/* Console */
extern void syspage_consoleSet(unsigned int id);


#endif
