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


#define NAME_PART_DEFAULT "default"


/* clang-format off */
enum { flagSyspageExec = 0x01, flagSyspageNoCopy = 0x02 };
/* clang-format on */


typedef struct {
	unsigned short width;
	unsigned short height;
	unsigned short bpp;
	unsigned short pitch;
	unsigned long framebuffer; /* addr_t */
} __attribute__((packed)) graphmode_t;


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


extern int syspage_mapAddrResolve(addr_t addr, const char **name);


extern const char *syspage_mapName(u8 id);


extern void syspage_mapShow(void);


extern mapent_t *syspage_entryAdd(const char *mapName, addr_t start, size_t size, unsigned int align);

/* Scheduler's functions */
extern int syspage_schedulerConfigSet(syspage_sched_t *config);


extern syspage_sched_t *syspage_schedulerConfigGet(void);

/* Partition's functions */
extern syspage_part_t *syspage_partAdd(void);


extern syspage_part_t *syspage_partsGet(void);


extern int syspage_partResolve(const char *partName, syspage_part_t **result);


/* Program's functions */
extern syspage_prog_t *syspage_progAdd(const char *argv, u32 flags);


/* This function allows syspage programs or blobs to be created at runtime.
 * Allocate `size` bytes in the memory map named `map`, then add a program named `argv` with `flags` into syspage.
 * If `allowOverwrite` is == 0, syspage will be checked for existing entries that match `argv` and allocation
 * will fail if an entry with the same `argv` already exists.
 * Returns pointer to allocated memory if successful, NULL if failed.
 * Caller can then write desired contents into the memory pointed by the return pointer.
 */
extern void *syspage_progAllocateAndAdd(const char *map, size_t size, const char *argv, u32 flags, int allowOverwrite);


extern void syspage_progShow(void);


/* Console */
extern void syspage_consoleSet(unsigned int id);


#if defined(HAS_GRAPHICS) && HAS_GRAPHICS != 0
/* Graphics mode */
extern void syspage_graphmodeSet(graphmode_t graphmode);
#endif


#endif
