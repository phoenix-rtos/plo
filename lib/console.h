/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * Console
 *
 * Copyright 2021 Phoenix Systems
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#define CONSOLE_CURSOR_HIDE "\033[?25l"
#define CONSOLE_CURSOR_SHOW "\033[?25h"

#define CONSOLE_CLEAR   "\033[2J\033[H"
#define CONSOLE_BOLD    "\033[0m\033[1m"
#define CONSOLE_RED     "\033[0m\033[31m"
#define CONSOLE_GREEN   "\033[0m\033[32m"
#define CONSOLE_MAGENTA "\033[0m\033[35m"
#define CONSOLE_CYAN    "\033[0m\033[36m"
#define CONSOLE_NORMAL  "\033[0m\033[37m"


/* Function sets device's major and minor number on which console should has output */
extern void console_set(unsigned major, unsigned minor);


/* Function prints string */
extern void console_puts(const char *s);


/* Function prints char */
extern void console_putc(char c);


/* Function gets char */
extern int console_getc(char *c, unsigned int timeout);


#endif
