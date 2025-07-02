/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Types
 *
 * Copyright 2021, 2022 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski, Aleksander Kaminski, Damian Loewnau
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

typedef vu8 *reg8;
typedef vu16 *reg16;
typedef vu32 *reg32;

typedef unsigned int addr_t;
typedef unsigned int size_t;
typedef int ssize_t;
typedef unsigned long long time_t;


#endif
