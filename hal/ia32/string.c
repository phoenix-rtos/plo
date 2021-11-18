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

#include <hal/string.h>


void *hal_memcpy(void *dst, const void *src, size_t n)
{
	void *d = dst;

	__asm__ volatile(
		"cld; "
		"rep; movsl; "
		"movl %%edx, %%ecx; "
		"rep; movsb; "
	: "+D" (d)
	: "c" (n >> 2), "d" (n & 3), "S" (src)
	: "memory", "cc");

	return dst;
}


int hal_memcmp(const void *ptr1, const void *ptr2, size_t num)
{
	int res;
	const char *p1 = ptr1;
	const char *p2 = ptr2;

	while (num--) {
		if ((res = *(p1++) - *(p2++)) != 0)
			return res < 0 ? -1 : 1;
	}

	return 0;
}


void *hal_memset(void *dst, int v, size_t n)
{
	void *d = dst;

	__asm__ volatile(
		"cld; "
		"movl %%eax, %%ebx; "
		"shll $0x8, %%ebx; "
		"orl %%ebx, %%eax; "
		"movl %%eax, %%ebx; "
		"shll $0x10, %%ebx; "
		"orl %%ebx, %%eax; "
		"rep; stosl; "
		"movl %%edx, %%ecx; "
		"rep; stosb; "
	: "+D" (d)
	: "a" (v & 0xff), "c" (n >> 2), "d" (n & 3)
	: "ebx", "memory", "cc");

	return dst;
}


size_t hal_strlen(const char *s)
{
	size_t i;

	for (i = 0; *s; s++, i++)
		;

	return i;
}


int hal_strcmp(const char *s1, const char *s2)
{
	unsigned int i;
	const char *p;

	for (p = s1, i = 0; *p; i++, p++) {
		if (*p < *(s2 + i))
			return -1;
		else if (*p > *(s2 + i))
			return 1;
	}

	if (*p != *(s2 + i))
		return -1;

	return 0;
}


int hal_strncmp(const char *s1, const char *s2, size_t n)
{
	size_t i;

	for (i = 0; (i < n) && *s1 && *s2 && (*s1 == *s2); i++, s1++, s2++)
		;

	if ((i == n) || (!*s1 && !*s2))
		return 0;

	return (*s1 < *s2) ? -(int)i - 1 : i + 1;
}


char *hal_strcpy(char *dst, const char *src)
{
	unsigned int i = 0;

	do {
		dst[i] = src[i];
	} while (src[i++]);

	return dst;
}


char *hal_strncpy(char *dst, const char *src, size_t n)
{
	unsigned int i = 0;

	do {
		dst[i] = src[i];
		i++;
	} while ((i < n) && src[i - 1]);

	return dst;
}
