/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ASCII string to integer conversion - code derived from libphoenix
 *
 * Copyright 2017-2018, 2021 Phoenix Systems
 * Author: Jakub Sejdak, Pawel Pisarczyk, Michał Mirosław, Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"

#include <hal/hal.h>


unsigned long lib_strtoul(char *nptr, char **endptr, int base)
{
	unsigned long t, v = 0;

	if (((base == 16) || (base == 0)) && (nptr[0] == '0') && ((nptr[1] | 0x20) == 'x')) {
		base = 16;
		nptr += 2;
	}
	else if ((base == 0) && (nptr[0] == '0')) {
		base = 8;
	}
	else if (base == 0) {
		base = 10;
	}

	for (; lib_isdigit(*nptr) || lib_isalpha(*nptr); nptr++) {
		t = *nptr - '0';
		if (t > 9)
			t = (*nptr | 0x20) - 'a' + 10;

		if (t >= (unsigned long)base)
			break;

		v = (v * base) + t;
	}

	if (endptr != NULL)
		*endptr = nptr;

	return v;
}


long lib_strtol(char *nptr, char **endptr, int base)
{
	int sign = 1;

	if (*nptr == '-') {
		sign = -1;
		++nptr;
	}

	return sign * lib_strtoul(nptr, endptr, base);
}
