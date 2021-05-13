/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * printf - code derived from libphoenix
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Adrian Kepka, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "hal.h"
#include "lib.h"
#include "format.h"
#include "console.h"


static void lib_printfFeed(void *context, char c)
{
	size_t *n = context;
	*n = *n + 1;
	console_putc(c);
}


int lib_printf(const char *format, ...)
{
	va_list arg;
	size_t n = 0;

	va_start(arg, format);
	format_parse(&n, lib_printfFeed, format, arg);
	va_end(arg);

	return n;
}
