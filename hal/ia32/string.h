/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * HAL basic routines
 *
 * Copyright 2012, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_BASE_H_
#define _HAL_BASE_H_

#include "types.h"


extern void *hal_memcpy(void *dst, const void *src, size_t n);


extern void *hal_memset(void *dst, int v, size_t n);


extern size_t hal_strlen(const char *s);


extern int hal_strcmp(const char *s1, const char *s2);


extern int hal_strncmp(const char *s1, const char *s2, size_t n);


extern char *hal_strcpy(char *dst, const char *src);


extern char *hal_strncpy(char *dst, const char *src, size_t n);


extern unsigned int hal_i2s(char *prefix, char *s, unsigned int n, unsigned char base, unsigned char zero);


#endif
