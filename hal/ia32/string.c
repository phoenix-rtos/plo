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

#include "string.h"


void *hal_memcpy(void *dst, const void *src, size_t n)
{
	__asm__ volatile(
		"cld; "
		"movl %0, %%ecx; "
		"movl %%ecx, %%edx; "
		"andl $3, %%edx; "
		"shrl $2, %%ecx; "
		"movl %1, %%edi; "
		"movl %2, %%esi; "
		"rep; movsl; "
		"movl %%edx, %%ecx; "
		"rep; movsb; "
	:
	: "g" (n), "g" (dst), "g" (src)
	: "ecx", "edx", "esi", "edi", "memory", "cc");

	return dst;
}


void *hal_memset(void *dst, int v, size_t n)
{
	__asm__ volatile(
		"cld; "
		"movl %0, %%ecx; "
		"movl %%ecx, %%edx; "
		"andl $3, %%edx; "
		"shrl $2, %%ecx; "
		"xorl %%eax, %%eax; "
		"movb %1, %%al; "
		"movl %%eax, %%ebx; "
		"shll $8, %%ebx; "
		"orl %%ebx, %%eax; "
		"movl %%eax, %%ebx; "
		"shll $16, %%ebx; "
		"orl %%ebx, %%eax; "
		"movl %2, %%edi; "
		"rep; stosl; "
		"movl %%edx, %%ecx; "
		"rep; stosb; "
	: "+d" (n)
	: "m" (v), "m" (dst)
	: "eax", "ebx", "ecx", "edi" ,"memory", "cc");

	return dst;
}


size_t hal_strlen(const char *s)
{
	size_t i;

	for (i = 0; *s; s++, i++);

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

	for (i = 0; (i < n) && *s1 && *s2 && (*s1 == *s2); i++, s1++, s2++);

	if ((i == n) || (!*s1 && !*s2))
		return 0;

	return (*s1 < *s2) ? -(int)i - 1 : i + 1;
}


char *hal_strcpy(char *dst, const char *src)
{
	unsigned int i = 0;

	do {
		dst[i] = src[i];
	} while(src[i++]);

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


unsigned int hal_i2s(char *prefix, char *s, unsigned int n, unsigned char base, unsigned char zero)
{
	static const char digits[] = "0123456789abcdef";
	unsigned int l, k, m = 0;
	char c;

	if (prefix != NULL) {
		while (*prefix)
			s[m++] = *prefix++;
	}

	for (k = m, l = (unsigned int)-1; l; n /= base, l /= base) {
		if (!zero && !n)
			break;
		s[k++] = digits[n % base];
	}
	l = k--;

	while (k > m) {
		c = s[m];
		s[m++] = s[k];
		s[k--] = c;
	}

	return l;
}
