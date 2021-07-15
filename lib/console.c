/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lib.h"

#include <hal/hal.h>
#include <devices/devs.h>


struct {
	int init;
	unsigned int major;
	unsigned int minor;
} console_common;


void lib_consolePuts(const char *s)
{
	if (!console_common.init) {
		hal_consolePrint(s);
		return;
	}

	devs_write(console_common.major, console_common.minor, 0, s, hal_strlen(s));
}


void lib_consolePutc(char c)
{
	const char data[] = { c, '\0' };

	if (!console_common.init) {
		hal_consolePrint(data);
		return;
	}

	devs_write(console_common.major, console_common.minor, 0, &data, 1);
}


int lib_consoleGetc(char *c, time_t timeout)
{
	if (!console_common.init) {
		lib_consolePuts(CONSOLE_RED);
		lib_consolePuts("\rCan't get data from console.");
		lib_consolePuts("\nPlease reset plo and set console to device.");
		for (;;);
	}

	return devs_read(console_common.major, console_common.minor, 0, c, 1, timeout);
}


void lib_consoleSet(unsigned major, unsigned minor)
{
	console_common.major = major;
	console_common.minor = minor;
	console_common.init = 1;
	lib_printf("\nconsole: Setting console to %d.%d", major, minor);
}
