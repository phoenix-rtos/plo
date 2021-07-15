/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Send formatted data to console - code derived from libphoenix
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Adrian Kepka, Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"

#include <hal/hal.h>


/* Format parser */
extern void lib_formatParse(void *ctx, void (*feed)(void *, char), const char *format, va_list args);


static void lib_printfFeed(void *context, char c)
{
	(*(size_t *)context)++;
	lib_consolePutc(c);
}


int lib_printf(const char *format, ...)
{
	va_list arg;
	size_t n = 0;

	va_start(arg, format);
	lib_formatParse(&n, lib_printfFeed, format, arg);
	va_end(arg);

	return n;
}
