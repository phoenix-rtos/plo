/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * HAL basic routines
 *
 * Copyright 2017, 2018, 2021 Phoenix Systems
 * Author: Pawel Pisarczyk, Aleksander Kaminski, Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _HAL_STRING_H_
#define _HAL_STRING_H_

#include <config.h>


extern void *hal_memcpy(void *dst, const void *src, size_t l);


extern int hal_memcmp(const void *ptr1, const void *ptr2, size_t num);


extern void hal_memset(void *dst, int v, size_t l);


extern size_t hal_strlen(const char *s);


extern int hal_strcmp(const char *s1, const char *s2);


extern int hal_strncmp(const char *s1, const char *s2, size_t count);


extern char *hal_strcpy(char *dest, const char *src);


extern char *hal_strncpy(char *dest, const char *src, size_t n);


extern char *hal_strchr(const char *str, int z);


extern int hal_i2s(char *prefix, char *s, unsigned int i, unsigned char b, char zero);


#endif
