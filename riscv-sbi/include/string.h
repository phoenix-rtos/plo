/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * String functions
 *
 * Copyright 2017, 2018, 2021, 2024 Phoenix Systems
 * Author: Pawel Pisarczyk, Aleksander Kaminski, Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_STRING_H_
#define _SBI_STRING_H_


#include "types.h"


void *sbi_memcpy(void *dst, const void *src, size_t l);


int sbi_memcmp(const void *ptr1, const void *ptr2, size_t num);


void sbi_memset(void *dst, int v, size_t l);


size_t sbi_strlen(const char *s);


int sbi_strcmp(const char *s1, const char *s2);


int sbi_strncmp(const char *s1, const char *s2, size_t count);


char *sbi_strcpy(char *dest, const char *src);


char *sbi_strncpy(char *dest, const char *src, size_t n);


char *sbi_strchr(const char *str, int z);


int sbi_i2s(char *prefix, char *s, unsigned long i, unsigned char b, char zero);


#endif
