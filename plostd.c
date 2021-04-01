/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Standard functions
 *
 * Copyright 2012 Phoenix Systems
 * Copyright 2001, 2005, 2020 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "errors.h"
#include "hal.h"
#include "plostd.h"


int plostd_ishex(const char *s)
{
	while (*s != '\0') {
		if (!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F')))
			return -1;
		s++;
	}

	return 1;
}


int plostd_isalnum(char c)
{
	/* test digit */
	if ((c >= '0') && (c <= '9'))
		return 1;

	/* test letter */
	c &= ~0x20;
	if ((c >= 'A') && (c <= 'Z'))
		return 1;

	return 0;
}


unsigned int plostd_strlen(const char *s)
{
	unsigned int k = 0;

	for (; s[k]; k++);
	return k;
}


int plostd_strcmp(const char *s1, const char *s2)
{
	const char *p;
	unsigned int k;

	for (p = s1, k = 0; *p; p++, k++) {

		if (*p < *(s2 + k))
			return -1;
		else if (*p > *(s2 + k))
			return 1;
	}

	if (*p != *(s2 + k))
		return -1;

	return 0;
}


int plostd_strncmp(const char *s1, const char *s2, u32 n)
{
	const char *p;
	u32 k = 0;

	for (p = s1, k = 0; *p && k < n; p++, k++) {
		if (*p < *(s2 + k))
			return -1;
		else if (*p > *(s2 + k))
			return 1;
	}

	return 0;
}


char *plostd_itoa(unsigned int i, char *buff)
{
	static const char *digits = "0123456789";
	int l = 0;
	int div, offs;
	int nz = 0;

	switch (sizeof(i)) {
	case 1:
		div = 100;
		break;
	case 2:
		div = 10000;
		break;
	case 4:
		div = 1000000000;
		break;
	default:
		return NULL;
	}

	while (div) {
		if ((offs = i / div) != 0)
			nz = 1;
		if (nz) {
			buff[l] = digits[offs];
			l++;
		}
		i -= offs * div;
		div /= 10;
	}
	if (!l)
		buff[l++] = digits[offs];

	buff[l] = 0;
	return buff;
}


char *plostd_itoah(u8 *ip, u8 is, char *buff, int lz)
{
	static const char *digitsh = "0123456789abcdef";
	int l, offs, i, nz = 0;

	for (i = 0, l = 0; i < is; i++) {
		offs = (ip[is - 1 - i] >> 4) & 0xf;
		nz |= offs;
		if (lz || nz)
			buff[l++] = digitsh[offs];

		offs = (ip[is - 1 - i]) & 0xf;
		nz |= offs;
		if (lz || nz || (i == is - 1))
			buff[l++] = digitsh[offs];
	}

	buff[l] = 0;
	return buff;
}


static unsigned int _plostd_ltoa(unsigned long n, unsigned int base, char *buff)
{
	static const char *digits = "0123456789abcdef";
	unsigned int i = (n < base) ? 0 : _plostd_ltoa(n / base, base, buff);

	buff[i] = digits[n % base];
	return i + 1;
}


char *plostd_ltoa(unsigned long n, unsigned int base, char *buff)
{
	char *saved_buff = buff;

	if (base == 2) {
		buff[0] = '0';
		buff[1] = 'b';
		buff += 2;
	}
	else if (base == 8) {
		buff[0] = '0';
		buff[1] = 'o';
		buff += 2;
	}
	else if (base == 16) {
		buff[0] = '0';
		buff[1] = 'x';
		buff += 2;
	}

	buff[_plostd_ltoa(n, base, buff)] = '\0';
	return saved_buff;
}


unsigned int plostd_ahtoi(const char *s)
{
	static const char *digitsh = "0123456789abcdef";
	char *p;
	int k, i, found;
	unsigned int v, pow;

	v = 0;
	pow = 0;
	for (k = plostd_strlen(s) - 1; k >= 0; k--, pow++) {
		p = (char *)(s + k);
		if ((*p == ' ') || (*p == '\t'))
			continue;

		found = 0;
		for (i = 0; i < 16; i++)
			if (digitsh[i] == *(char *)p) {
				found = 1;
				break;
			}
		if (!found)
			return 0;

		if (pow > (sizeof(int) * 2 - 1))
			return v;
		v += (i << (pow * 4));
	}
	return v;
}


static unsigned long _plostd_atol(const char *s, unsigned int base, unsigned long n)
{
	unsigned int x;

	if (plostd_isalnum(*s)) {
		if ((x = *s - '0') > 9)
			x = (*s | 0x20) - 'a' + 10;

		if (x >= base)
			return ERR_ARG;

		return _plostd_atol(s + 1, base, n * base + x);
	}

	return n;
}


unsigned long plostd_atol(const char *s)
{
	unsigned int base = 10;

	if (s[0] == '0') {
		if (s[1] == 'b') {
			base = 2;
			s += 2;
		}
		else if (s[1] == 'o') {
			base = 8;
			s += 2;
		}
		else if (s[1] == 'x') {
			base = 16;
			s += 2;
		}
	}

	return _plostd_atol(s, base, 0UL);
}


void plostd_puts(const char *s)
{
	for (; *s; s++)
		hal_putc(*s);
	return;
}


void plostd_printf(char attr, const char *fmt, ...)
{
	va_list ap;
	const char *p;
	char buff[16];
	int i;
	long l;

	va_start(ap, fmt);

	if (attr != ATTR_NONE)
		hal_setattr(attr);

	for (p = fmt; *p; p++) {
		if (*p != '%') {
			hal_putc(*p);
			continue;
		}

		switch (*++p) {
		case 'd':
			i = va_arg(ap, int);
			plostd_puts(plostd_itoa(i, buff));
			break;
		case 'x':
			i = va_arg(ap, int);
			plostd_puts(plostd_itoah((u8 *)&i, 2, buff, 0));
			break;
		case 'p':
			i = va_arg(ap, int);
			plostd_puts(plostd_itoah((u8 *)&i, 4, buff, 1));
			break;
		case 'P':
			l = va_arg(ap, long);
			plostd_puts(plostd_itoah((u8 *)&l, 4, buff, 1));
			break;
		case 's':
			plostd_puts(va_arg(ap, char *));
			break;
		case 'c':
			hal_putc(va_arg(ap, int));
			break;
		case '%':
			hal_putc('%');
			break;
		}
	}
	va_end(ap);

	/* CSI normal: all attributes off */
	if (attr != ATTR_NONE)
		plostd_puts("\033[0m");

	return;
}
