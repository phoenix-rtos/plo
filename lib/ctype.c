/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Character type - code derived from libphoenix
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Adrian Kepka, Hubert Buczynski, Gerard Swiderski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "ctype.h"


int lib_islower(int c)
{
	return (c >= 'a') && (c <= 'z');
}


int lib_isupper(int c)
{
	return (c >= 'A') && (c <= 'Z');
}


int lib_isalpha(int c)
{
	return lib_islower(c) || lib_isupper(c);
}


int lib_isdigit(int c)
{
	return (c >= '0') && (c <= '9');
}


int lib_isblank(int c)
{
	return (c == ' ') || (c == '\t');
}


int lib_isspace(int c)
{
	return lib_isblank(c) || ((c >= '\n') && (c <= '\r'));
}


int lib_isgraph(int c)
{
	return (c > ' ') && (c < 0x7f);
}


int lib_isprint(int c)
{
	return (c >= ' ') && (c < 0x7f);
}
