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
	char *endptr;
	char word[SIZE_CMD_ARG_LINE + 1];
	unsigned int p = 0;
	int xsize = 16;
	int ysize = 16;
	unsigned int x, y;
	u32 offs;
	u8 b;

	/* Get address */
	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL) {
		lib_printf("\nSize error!\n");
		return ERR_ARG;
	}
	if (*word == 0) {
		lib_printf("\nBad segment!\n");
		return ERR_ARG;
	}

	offs = lib_strtoul(word, &endptr, 16);
	if (hal_strlen(endptr) != 0) {
		lib_printf("\nWrong address value: %s", word);
		return ERR_ARG;
	}

	lib_printf("\nMemory dump from 0x%x:\n", offs);
	lib_printf("--------------------------\n");

	for (y = 0; y < ysize; y++) {
		lib_printf("0x%x   ", offs);

		/* Print byte values */
		for (x = 0; x < xsize; x++) {
			b = *(u8 *)(offs + x);
			if (b & 0xf0)
				lib_printf("%x ", b);
			else
				lib_printf("0%x ", b);
		}
		lib_printf("  ");

		/* Print ASCII representation */
		for (x = 0; x < xsize; x++) {
			b = *(u8 *)(offs + x);
			if ((b <= 32) || (b > 127))
				lib_printf(".", b);
			else
				lib_printf("%c", b);
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
