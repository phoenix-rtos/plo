/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * HAL basic routines
 *
 * Copyright 2012, 2024 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */
#include <hal/string.h>


void *hal_memcpy(void *dst, const void *src, size_t l)
{
	char *dst_c = (char *)dst;
	const char *src_c = (const char *)src;
	char *dst_end = dst_c + l;

	while (dst_c != dst_end) {
		*dst_c = *src_c;
		dst_c++;
		src_c++;
	}

	return dst;
}


void hal_memset(void *dst, int val, size_t len)
{
	size_t i;

	for (i = 0; i < len; ++i)
		((char *)dst)[i] = (char)val;
}


int hal_memcmp(const void *ptr1, const void *ptr2, size_t num)
{
	int i;

	for (i = 0; i < num; ++i) {
		if (((const u8 *)ptr1)[i] < ((const u8 *)ptr2)[i]) {
			return -1;
		}
		else if (((const u8 *)ptr1)[i] > ((const u8 *)ptr2)[i]) {
			return 1;
		}
	}

	return 0;
}


size_t hal_strlen(const char *s)
{
	size_t k;

	for (k = 0; *s != '\0'; s++, k++) {
	}

	return k;
}


int hal_strcmp(const char *s1, const char *s2)
{
	const unsigned char *us1 = (const unsigned char *)s1;
	const unsigned char *us2 = (const unsigned char *)s2;
	const unsigned char *p;
	unsigned int k;

	for (p = us1, k = 0; *p != '\0'; p++, k++) {

		if (*p < *(us2 + k)) {
			return -1;
		}
		else if (*p > *(us2 + k)) {
			return 1;
		}
	}

	if (*p != *(us2 + k)) {
		return -1;
	}

	return 0;
}


int hal_strncmp(const char *s1, const char *s2, size_t count)
{
	const unsigned char *us1 = (const unsigned char *)s1;
	const unsigned char *us2 = (const unsigned char *)s2;
	size_t k;

	for (k = 0; (k < count) && (*us1 != '\0') && (*us2 != '\0') && (*us1 == *us2); ++k, ++us1, ++us2) {
	}

	if ((k == count) || ((*us1 == '\0') && (*us2 == '\0'))) {
		return 0;
	}

	return (*us1 < *us2) ? (-k - 1) : (k + 1);
}


char *hal_strcpy(char *dest, const char *src)
{
	int i = 0;

	do {
		dest[i] = src[i];
	} while (src[i++] != '\0');

	return dest;
}


char *hal_strncpy(char *dest, const char *src, size_t n)
{
	int i = 0;

	if (n == 0) {
		return dest;
	}

	do {
		dest[i] = src[i];
		i++;
	} while ((i < n) && (src[i - 1] != '\0'));

	return dest;
}


char *hal_strchr(const char *s, int c)
{
	do {
		if (*s == c) {
			return (char *)s;
		}
	} while (*s++ != '\0');

	return NULL;
}


int hal_i2s(char *prefix, char *s, unsigned long i, unsigned char b, char zero)
{
	static const char digits[] = "0123456789abcdef";
	char c;
	unsigned int l, k, m;

	m = hal_strlen(prefix);
	hal_memcpy(s, prefix, m);

	for (k = m, l = (unsigned int)-1; l; i /= b, l /= b) {
		if (!zero && !i) {
			break;
		}
		s[k++] = digits[i % b];
	}

	l = k--;

	while (k > m) {
		c = s[m];
		s[m++] = s[k];
		s[k--] = c;
	}

	return l;
}
