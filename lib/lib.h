/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Loader library routines
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIB_LIB_H_
#define _LIB_LIB_H_

#include "cbuffer.h"
#include "console.h"
#include "ctype.h"
#include "errno.h"
#include "getopt.h"
#include "list.h"
#include "log.h"
#include "stdarg.h"
#include "prompt.h"
#include "crc32.h"


#define min(a, b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a > _b ? _b : _a; \
})


#define max(a, b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a > _b ? _a : _b; \
})


extern int lib_printf(const char *fmt, ...);


extern int lib_sprintf(char *str, const char *fmt, ...);


extern unsigned long lib_strtoul(char *nptr, char **endptr, int base);


extern long lib_strtol(char *nptr, char **endptr, int base);


#endif
