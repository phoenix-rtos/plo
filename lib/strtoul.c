/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * Standard routines - ASCII to integer conversion
 *
 * Copyright 2017 Phoenix Systems
 * Author: Jakub Sejdak, Aleksander Kaminski, Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "lib.h"
#include "types.h"


static int strtoul_isalnum(char c)
{
	if ((c >= '0') && (c <= '9'))
		return 1;

	/* test letter */
	c &= ~0x20;
	if ((c >= 'A') && (c <= 'Z'))
		return 1;

	return 0;
}


unsigned int lib_strtoul(char *nptr, char **endptr, int base)
{
	unsigned int t, v = 0;

	if (base == 16 && nptr[0] == '0' && nptr[1] == 'x')
		nptr += 2;

	while (strtoul_isalnum(*nptr)) {
		if ((t = *nptr - '0') > 9)
			t = (*nptr | 0x20) - 'a' + 10;

		if (t >= base)
			break;

		v = (v * base) + t;

		++nptr;
	}

	if (*endptr != NULL)
		*endptr = nptr;

	return v;
}


int lib_strtol(char *nptr, char **endptr, int base)
{
	int sign = 1;

	if (*nptr == '-') {
		sign = -1;
		++nptr;
	}

	return sign * lib_strtoul(nptr, endptr, base);
}