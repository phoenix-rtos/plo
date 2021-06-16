/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * dump command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>


static void cmd_dumpInfo(void)
{
	lib_printf("dumps memory, usage: dump <addr>");
}


/* TODO: old code needs to be cleaned up; address has to be checked with maps */
static int cmd_dump(int argc, char *argv[])
{
	u8 byte;
	addr_t offs;
	char *endptr;
	unsigned int x, y;
	static const int xsize = 16, ysize = 16;

	if (argc == 1) {
		log_error("\n%s: Arguments have to be defined", argv[0]);
		return -EINVAL;
	}

	offs = lib_strtoul(argv[1], &endptr, 16);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return -EINVAL;
	}

	lib_printf("\nMemory dump from 0x%x:\n", offs);
	lib_printf("--------------------------\n");

	for (y = 0; y < ysize; y++) {
		lib_printf("0x%x   ", offs);

		/* Print byte values */
		for (x = 0; x < xsize; x++) {
			byte = *(u8 *)(offs + x);
			if (byte & 0xf0)
				lib_printf("%x ", byte);
			else
				lib_printf("0%x ", byte);
		}
		lib_printf("  ");

		/* Print ASCII representation */
		for (x = 0; x < xsize; x++) {
			byte = *(u8 *)(offs + x);
			if ((byte <= 32) || (byte > 127))
				lib_printf(".", byte);
			else
				lib_printf("%c", byte);
		}

		lib_printf("\n");
		offs += xsize;
	}

	return EOK;
}


__attribute__((constructor)) static void cmd_dumpReg(void)
{
	const static cmd_t app_cmd = { .name = "dump", .run = cmd_dump, .info = cmd_dumpInfo };

	cmd_reg(&app_cmd);
}
