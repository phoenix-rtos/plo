/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * Standard loader library
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIB_PLO_H_
#define _LIB_PLO_H_

#include <hal/hal.h>


#define min(a, b) ((a > b) ? b : a)


extern int lib_printf(const char *fmt, ...);


extern void lib_putch(char c);


extern unsigned int lib_strtoul(char *nptr, char **endptr, int base);


extern int lib_strtol(char *nptr, char **endptr, int base);


#endif
