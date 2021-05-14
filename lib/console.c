/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-loader
 *
 * Console
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "hal.h"
#include "devs.h"
#include "console.h"
#include "errors.h"


struct {
	int init;
	unsigned int major;
	unsigned int minor;
} console_common;


void console_puts(const char *s)
{
	if (!console_common.init) {
		hal_consolePrint(s);
		return;
	}

	devs_write(console_common.major, console_common.minor, 0, (const u8 *)s, hal_strlen(s));
}


void console_putc(char c)
{
	const char data[] = {c, '\0'};

	if (!console_common.init) {
		hal_consolePrint(data);
		return;
	}

	devs_write(console_common.major, console_common.minor, 0, (const u8 *)&data, 1);
}


int console_getc(char *c, unsigned int timeout)
{
	if (!console_common.init)
		return ERR_ARG;

	return devs_read(console_common.major, console_common.minor, 0, (u8 *)c, 1, timeout);
}


void console_set(unsigned major, unsigned minor)
{
	console_common.major = major;
	console_common.minor = minor;
	console_common.init = 1;
}
