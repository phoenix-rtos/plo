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


enum { flagSyspageExec = 0x01 };


enum { mAttrRead = 0x01, mAttrWrite = 0x02, mAttrExec = 0x04, mAttrShareable = 0x08,
	   mAttrCacheable = 0x10, mAttrBufferable = 0x20 };


enum { console_default = 0, console_com0, console_com1, console_com2, console_com3, console_com4, console_com5, console_com6,
	   console_com7, console_com8, console_com9, console_com10, console_com11, console_com12, console_com13, console_com14,
	   console_com15, console_vga0 };


#pragma pack(push, 1)

typedef struct _syspage_prog_t {
	struct _syspage_prog_t *next, *prev;

	addr_t start;
	addr_t end;

	char *argv;

	size_t imapSz;
	u8 *imaps;

	size_t dmapSz;
	u8 *dmaps;
} syspage_prog_t;


typedef struct _syspage_map_t {
	struct _syspage_map_t *next, *prev;

	mapent_t *entries;

	addr_t start;
	addr_t end;

	u32 attr;
	u8 id;

	char *name;
} syspage_map_t;


typedef struct {
	hal_syspage_t hs;

	syspage_map_t *maps;
	syspage_prog_t *progs;

	size_t size;
	unsigned int console;
} syspage_t;

#pragma pack(pop)


/* General functions */
extern void syspage_init(void);


extern void *syspage_alloc(size_t size);


/* Map's functions */
extern int syspage_mapAdd(const char *name, addr_t start, addr_t end, const char *attr);


extern int syspage_mapAttrResolve(const char *name, unsigned int *attr);


extern int syspage_mapNameResolve(const char *name, u8 *id);


extern int syspage_mapRangeResolve(const char *name, addr_t *start, addr_t *end);


extern const char *syspage_mapName(u8 id);


extern void syspage_mapShow(void);


extern mapent_t *syspage_entryAdd(const char *mapName, addr_t start, size_t size, unsigned int align);


/* Program's functions */
extern syspage_prog_t *syspage_progAdd(const char *argv, u32 flags);


extern void syspage_progShow(void);


/* Console */
extern void syspage_consoleSet(unsigned int id);


#endif
