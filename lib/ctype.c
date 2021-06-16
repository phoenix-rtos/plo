/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * ctype - code derived from libphoenix
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Adrian Kepka, Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"


int islower(int c)
{
	return c >= 'a' && c <= 'z';
}


int isupper(int c)
{
	return c >= 'A' && c <= 'Z';
}


int isalpha(int c)
{
	return islower(c) || isupper(c);
}


int isdigit(int c)
{
	return c >= '0' && c <= '9';
}


int isblank(int c)
{
	return c == '\t' || c == ' ';
}


int isspace(int c)
{
	return isblank(c) || (c >= '\n' && c <= '\r');
}


int isgraph(int c)
{
	return c > ' ' && c < 0x7f;
}


int isprint(int c)
{
	return c >= ' ' && c < 0x7f;
}
