/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * ctype
 *
 * Copyright 2021 Phoenix Systems
 * Author:  Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"


int lib_ishex(const char *s)
{
	while (*s != '\0') {
		if (!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F')))
			return -1;
		s++;
	}

	return 1;
}
