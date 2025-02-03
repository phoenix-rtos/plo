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
	struct {
		unsigned int major;
		unsigned int minor;
	} mirrors[CONSOLE_MIRRORS];
	size_t mirrorsCnt;
	unsigned int major;
	unsigned int minor;
	ssize_t (*readHook)(int, void *, size_t);
	ssize_t (*writeHook)(int, const void *, size_t);
} console_common = { 0 };


void lib_consoleSetHooks(ssize_t (*rd)(int, void *, size_t), ssize_t (*wr)(int, const void *, size_t))
{
	console_common.readHook = rd;
	console_common.writeHook = wr;
}


static void lib_consoleWrite(const char *s, size_t len)
{
	/*
	 * FIXME: reversing order of writes breaks the mirroring
	 *        after second user input character on ia32 when mirroring from VGA to UART.
	 */
	size_t i;
	for (i = 0; i < console_common.mirrorsCnt; i++) {
		devs_write(console_common.mirrors[i].major, console_common.mirrors[i].minor, 0, s, len);
	}
	devs_write(console_common.major, console_common.minor, 0, s, len);
}


void lib_consolePuts(const char *s)
{
	size_t len;

	if (console_common.init == 0) {
		hal_consolePrint(s);
		return;
	}

	len = hal_strlen(s);
	if (console_common.writeHook != NULL) {
		console_common.writeHook(0, s, len);
	}

	lib_consoleWrite(s, len);
}


void lib_consolePutc(char c)
{
	const char data[] = { c, '\0' };

	if (!console_common.init) {
		hal_consolePrint(data);
		return;
	}

	if (console_common.writeHook != NULL) {
		console_common.writeHook(0, &data, 1);
	}

	lib_consoleWrite(data, 1);
}


int lib_consoleGetc(char *c, time_t timeout)
{
	*c = 0;
	if (console_common.readHook != 0) {
		if (console_common.readHook(0, c, 1) > 0) {
			return 1;
		}
		if (timeout == (time_t)-1) {
			/* override infinite timeout to allow polling by the read hook */
			timeout = 20;
		}
	}

	if (console_common.init == 0) {
		lib_consolePuts(CONSOLE_RED);
		lib_consolePuts("\rCan't get data from console.");
		lib_consolePuts("\nPlease reset plo and set console to device.");
		for (;;) {
			hal_cpuHalt();
		}
	}

	return devs_read(console_common.major, console_common.minor, 0, c, 1, timeout);
}


void lib_consoleSet(unsigned int major, unsigned int minor)
{
	console_common.major = major;
	console_common.minor = minor;
	console_common.init = 1;
}


void lib_consoleSetMirrors(size_t cnt, const unsigned int *majors, const unsigned int *minors)
{
	size_t i;
	cnt = min(CONSOLE_MIRRORS, cnt);
	for (i = 0; i < cnt; i++) {
		console_common.mirrors[i].major = majors[i];
		console_common.mirrors[i].minor = minors[i];
	}
	console_common.mirrorsCnt = cnt;
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
