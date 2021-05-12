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

#define TEXT_BOLD     "\033[0m\033[1m"
#define TEXT_RED      "\033[0m\033[31m"
#define TEXT_GREEN    "\033[0m\033[32m"
#define TEXT_MAGENTA  "\033[0m\033[35m"
#define TEXT_CYAN     "\033[0m\033[36m"
#define TEXT_NORMAL   "\033[0m\033[37m"


#define min(a, b)   ((a > b) ? b : a)


extern int lib_printf(const char *fmt, ...);


extern void lib_putch(char c);


extern int lib_ishex(const char *s);


extern unsigned int lib_strtoul(char *nptr, char **endptr, int base);


extern int lib_strtol(char *nptr, char **endptr, int base);


#endif
