/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Syspage
 *
 * Copyright 2020 Phoenix Systems
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef SYSPAGE_H_
#define SYSPAGE_H_

#include "../types.h"


/* TODO: Make it compatible with Phoenix-RTOS kernel;
 *       take into account map's attributes while data is written to them */
enum { mAttrRead = 0x01, mAttrWrite = 0x02, maAttrExec = 0x04, mAttrShareable = 0x08,
	   mAttrCacheable = 0x10, mAttrBufferable = 0x20 };



/* Initialization function */
extern void syspage_init(void);


/* Syspage's location functions */

extern int syspage_setAddress(void *addr);


extern void *syspage_getAddress(void);



/* General functions */

extern void syspage_save(void);


extern void syspage_show(void);



/* Map's functions */

extern int syspage_addmap(const char *name, void *start, void *end, u32 attr);


extern int syspage_getMapTop(const char *map, void **addr);


extern int syspage_alignMapTop(const char *map);


extern int syspage_getFreeSize(const char *map, u32 *sz);


extern int syspage_write2Map(const char *map, const u8 *buff, u32 len);


extern void syspage_addEntries(u32 start, u32 sz);



/* Program's functions */

extern int syspage_addProg(void *start, void *end, const char *imap, const char *dmap, const char *name);


/* Setting kernel's data */

extern void syspage_setKernelText(void *addr, u32 size);


extern void syspage_setKernelBss(void *addr, u32 size);


extern void syspage_setKernelData(void *addr, u32 size);


#endif
