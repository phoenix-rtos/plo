/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
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


#define CONSOLE_TIMEOUT_MS   100


extern void console_init(void);


extern void console_set(unsigned major, unsigned minor);


extern void console_puts(const char *s);


extern void console_putc(char c);


extern int console_getc(char *c);


#endif
