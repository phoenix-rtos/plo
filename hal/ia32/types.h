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


#define NULL ((void *)0)

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


#endif
