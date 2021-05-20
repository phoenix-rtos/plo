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
#include "hal.h"
#include "lib.h"


static void cmd_dumpInfo(void)
{
	lib_printf("dumps memory, usage: dump <segment>:<offset>");
}


/* TODO: old code needs to be cleaned up; address has to be checked with maps */
static int cmd_dump(char *s)
{
	u8 byte;
	addr_t offs;
	char *endptr;
	unsigned int x, y;
	static const int xsize = 16, ysize = 16;

	unsigned int argsc;
	char (*args)[SIZE_CMD_ARG_LINE];

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	if (argsc == 0) {
		log_error("\nArguments have to be defined");
		return ERR_ARG;
	}

	offs = lib_strtoul(args[0], &endptr, 16);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong address value: %s", args[0]);
		return ERR_ARG;
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

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_dumpReg(void)
{
	const static cmd_t app_cmd = { .name = "dump", .run = cmd_dump, .info = cmd_dumpInfo };

	cmd_reg(&app_cmd);
}
