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
	unsigned int major;
	unsigned int minor;
} consol_common;


void console_puts(const char *s)
{
	if (consol_common.major == -1 || consol_common.minor == -1) {
		hal_consolePrint(s);
		return;
	}

	devs_write(consol_common.major, consol_common.minor, 0, (const u8 *)s, hal_strlen(s));
}


void console_putc(char c)
{
	const char data[] = {c, '\0'};

	if (consol_common.major == -1 || consol_common.minor == -1) {
		hal_consolePrint(data);
		return;
	}

	devs_write(consol_common.major, consol_common.minor, 0, (const u8 *)&data, 1);
}


int console_getc(char *c)
{
	if (consol_common.major == -1 || consol_common.minor == -1)
		return ERR_ARG;

	return devs_read(consol_common.major, consol_common.minor, 0, (u8 *)c, 1, CONSOLE_TIMEOUT_MS);
}


void console_set(unsigned major, unsigned minor)
{
	consol_common.major = major;
	consol_common.minor = minor;
}


void console_init(void)
{
	consol_common.major = -1;
	consol_common.minor = -1;
}
