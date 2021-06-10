/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
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


#define SIZE_APP_NAME 15
#define SIZE_MAP_NAME 7


/* TODO: Make it compatible with Phoenix-RTOS kernel;
 *       take into account map's attributes while data is written to them */
enum { mAttrRead = 0x01, mAttrWrite = 0x02, mAttrExec = 0x04, mAttrShareable = 0x08,
	   mAttrCacheable = 0x10, mAttrBufferable = 0x20 };


/* syspage_addProg bitflags */
enum { flagSyspageExec = 0x01 };


/* Initialization function */
extern void syspage_init(void);


/* Syspage's location functions */

extern void syspage_setAddress(void *addr);


extern void *syspage_getAddress(void);


/* General functions */

extern int syspage_save(void);


extern void syspage_showAddr(void);


extern void syspage_showMaps(void);


extern void syspage_showApps(void);


extern void syspage_showKernel(void);


/* Map's functions */

extern int syspage_addmap(const char *name, void *start, void *end, const char *attr);


extern int syspage_getMapTop(const char *map, void **addr);


extern int syspage_alignMapTop(const char *map);


extern int syspage_getFreeSize(const char *map, size_t *sz);


extern int syspage_write2Map(const char *map, const u8 *buff, size_t len);


extern void syspage_addEntries(addr_t start, size_t sz);


extern int syspage_getMapAttr(const char *map, unsigned int *attr);


/* Program's functions */

extern int syspage_addProg(void *start, void *end, const char *imap, const char *dmap, const char *name, u32 flags);


/* Setting kernel's data */

extern void syspage_setKernelText(void *addr, size_t size);


extern void syspage_setKernelBss(void *addr, size_t size);


extern void syspage_setKernelData(void *addr, size_t size);


/* Add specific hal data */

extern void syspage_setHalData(const syspage_hal_t *hal);


#endif
