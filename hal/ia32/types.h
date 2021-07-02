/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform type extensions
 *
 * Copyright 2021 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TYPES_H_
#define _TYPES_H_


#define NULL 0

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile unsigned long long vu64;

typedef vu8 *reg8;
typedef vu16 *reg16;
typedef vu32 *reg32;
typedef vu64 *reg64;

typedef unsigned long long time_t;
typedef unsigned int addr_t;
typedef unsigned int size_t;
typedef int ssize_t;


/* FIXME: temporary memory entry definition until new syspage implementation */
typedef struct {
	unsigned long long addr;
	unsigned long long len;
	unsigned int type;
} mem_entry_t;


typedef struct {
	struct {
		unsigned short size;
		unsigned int addr;
	} __attribute__((packed)) gdtr;
	unsigned short pad1;
	struct {
		unsigned short size;
		unsigned int addr;
	} __attribute__((packed)) idtr;
	unsigned short pad2;
	unsigned int pdir;
	unsigned int ptable;
	unsigned int stack;
	unsigned int stacksz;

	/* FIXE: remove fields below after new syspage implementation is added */
	unsigned int kernel;
	unsigned int kernelsz;
	unsigned int console;
	char arg[256];

	unsigned short mmsize;
	struct {
		unsigned int addr;
		unsigned int res1;
		unsigned int len;
		unsigned int res2;
		unsigned short attr;
		unsigned short res3;
	} mm[64];

	unsigned short progssz;
	struct {
		unsigned int entry;
		unsigned int *hdrssz;
		struct {
			unsigned int addr;
			unsigned int size;
			unsigned int flags;
			unsigned int vaddr;
			unsigned int filesz;
			unsigned int padding;
		} hdrs[];
	} progs[];
} syspage_hal_t;


#endif
