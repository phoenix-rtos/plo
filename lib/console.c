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
		for (;;)
			;
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


void lib_consolePutHLine(void)
{
	unsigned int col;
	for (col = 0; col < 79u; ++col) {
		lib_consolePutc('-');
	}
	lib_consolePutc('\n');
}


void lib_consolePutRegionHex(addr_t start, addr_t end, addr_t offp, u8 align, unsigned int (*validator)(addr_t, addr_t, addr_t))
{
	unsigned int col;
	static const unsigned int cols = 16;
	addr_t ptr, offs = start;
	char c;

	if (align != 0) {
		offp = (offp / cols) * cols;
		offs = (offs / cols) * cols;
	}

	while (offs < end) {
		(void)lib_printf("0x%08x | ", (u32)offp);

		/* Print byte values */
		for (col = 0; col < cols; ++col) {
			ptr = offs + col;

			if ((ptr < start) || (ptr >= end)) {
				lib_consolePuts("   ");
				continue;
			}

			if ((validator != NULL) && (validator(start, end, ptr) == 0u)) {
				lib_consolePuts("XX ");
			}
			else {
				(void)lib_printf("%02x ", *(u8 *)ptr);
			}
		}
		lib_consolePuts("| ");

		/* Print "printable" representation */
		for (col = 0; col < cols; ++col) {
			ptr = offs + col;

			if ((ptr < start) || (ptr >= end)) {
				c = ' ';
			}
			else if ((validator != NULL) && (validator(start, end, ptr) == 0u)) {
				c = 'X';
			}
			else if (lib_isprint(*(u8 *)ptr) != 0) {
				c = *(u8 *)ptr;
			}
			else {
				c = '.';
			}

			lib_consolePutc(c);
		}

		lib_consolePutc('\n');
		offp += cols;
		offs += cols;
	}
}
