/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Send formatted data to string buffer
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Adrian Kepka, Krystian Wasik, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"

#include <hal/hal.h>


typedef struct {
	char *buff;
	size_t n;
} sprintf_ctx_t;


/* Format parser */
extern void lib_formatParse(void *ctx, void (*feed)(void *, char), const char *format, va_list args);


static void lib_sprintfFeed(void *context, char c)
{
	sprintf_ctx_t *ctx = (sprintf_ctx_t *)context;
	ctx->buff[ctx->n++] = c;
}


int lib_sprintf(char *str, const char *format, ...)
{
	sprintf_ctx_t ctx = { str, 0 };
	va_list arg;

	va_start(arg, format);
	lib_formatParse(&ctx, lib_sprintfFeed, format, arg);
	lib_sprintfFeed(&ctx, '\0');
	va_end(arg);

	return ctx.n - 1;
}
