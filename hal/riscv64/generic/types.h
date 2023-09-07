/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Types
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
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
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile unsigned long long vu64;

typedef vu8 *reg8;
typedef vu16 *reg16;
typedef vu32 *reg32;
typedef vu64 *reg64;

typedef u64 addr_t;
typedef u64 size_t;
typedef s64 ssize_t;
typedef unsigned long long time_t;


#endif
