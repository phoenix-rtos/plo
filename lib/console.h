/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2021 Phoenix Systems
 * Authors: Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _LIB_CONSOLE_H_
#define _LIB_CONSOLE_H_

#include <hal/hal.h>


#define CONSOLE_CURSOR_HIDE "\033[?25l"
#define CONSOLE_CURSOR_SHOW "\033[?25h"

#define CONSOLE_CLEAR   "\033[2J\033[H"
#define CONSOLE_NORMAL  "\033[0m"
#define CONSOLE_BOLD    "\033[1m"
#define CONSOLE_RED     "\033[31m"
#define CONSOLE_GREEN   "\033[32m"
#define CONSOLE_YELLOW  "\033[33m"
#define CONSOLE_BLUE    "\033[34m"
#define CONSOLE_MAGENTA "\033[35m"
#define CONSOLE_CYAN    "\033[36m"
#define CONSOLE_WHITE   "\033[37m"


/* Sets console device */
extern void lib_consoleSet(unsigned major, unsigned minor);


/* Prints string */
extern void lib_consolePuts(const char *s);


/* Prints character */
extern void lib_consolePutc(char c);


/* Gets character */
extern int lib_consoleGetc(char *c, time_t timeout);


#endif
