/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Dump memory
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>
#include <syspage.h>


static void cmd_dumpInfo(void)
{
	lib_printf("dumps memory, usage: dump <addr> <size>");
}


static unsigned int region_validate(addr_t start, addr_t end, addr_t curr)
{
	unsigned int attr = 0;

	(void)start;
	(void)end;

	if (syspage_mapRangeCheck(curr, curr, &attr)) {
		attr = (attr & mAttrRead);
	}

	return attr != 0;
}


static void region_hexdump(addr_t start, addr_t end, unsigned int (*validate)(addr_t, addr_t, addr_t))
{
	static const int cols = 16;

	unsigned int col, row;
	unsigned int rows = (end - start + cols - 1) / cols;
	addr_t ptr, offs = (start / cols) * cols;

	lib_printf("\nMemory dump from 0x%x to 0x%x (%zu bytes):\n", (u32)start, (u32)end, end - start);

	for (col = 0; col < 79; ++col) {
		lib_consolePutc('-');
	}

	lib_printf("\n");

	for (row = 0; row < rows; ++row) {
		lib_printf("0x%08x | ", (u32)offs);

		/* Print byte values */
		for (col = 0; col < cols; ++col) {
			ptr = offs + col;

			if ((ptr < start) || (ptr >= end)) {
				lib_printf("   ");
				continue;
			}

			if ((validate != NULL) && (validate(start, end, ptr) == 0)) {
				lib_printf("XX ");
			}
			else {
				lib_printf("%02x ", *(u8 *)ptr);
			}
		}
		lib_printf("| ");

		/* Print "printable" representation */
		for (col = 0; col < cols; ++col) {
			ptr = offs + col;

			if ((ptr < start) || (ptr >= end)) {
				lib_printf(" ");
				continue;
			}

			if ((validate != NULL) && (validate(start, end, ptr) == 0)) {
				lib_printf("X");
			}
			else if (lib_isprint(*(u8 *)ptr)) {
				lib_printf("%c", *(u8 *)ptr);
			}
			else {
				lib_printf(".");
			}
		}

		lib_printf("\n");
		offs += cols;
	}
}


static int cmd_dump(int argc, char *argv[])
{
	addr_t start;
	size_t length;
	char *endptr;

	if (argc == 2) {
		log_error("\n%s: Provide <address> and <size>", argv[0]);
		return -EINVAL;
	}

	start = lib_strtoul(argv[1], &endptr, 16);
	if (*endptr || ((start == 0) && (endptr == argv[1]))) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return -EINVAL;
	}

	length = lib_strtoul(argv[2], &endptr, 0);
	if (*endptr || ((length == 0) && (endptr == argv[2]))) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return -EINVAL;
	}

	if (start + length < start) {
		length = (addr_t)(-1) - start;
	}

	region_hexdump(start, start + length, region_validate);

	return EOK;
}


__attribute__((constructor)) static void cmd_dumpReg(void)
{
	const static cmd_t app_cmd = { .name = "dump", .run = cmd_dump, .info = cmd_dumpInfo };

	cmd_reg(&app_cmd);
}
