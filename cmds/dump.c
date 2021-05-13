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
#include "lib.h"


static void cmd_dumpInfo(void)
{
	lib_printf("dumps memory, usage: dump <segment>:<offset>");
}


static int cmd_dump(char *s)
{
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

	offs = lib_strtoul(word, NULL, 16);

	lib_printf("\n");
	lib_printf("Memory dump from %p:\n", offs);
	lib_printf("--------------------------\n");

	for (y = 0; y < ysize; y++) {
		lib_printf("%p   ", offs);

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

	lib_printf("");

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_dumpReg(void)
{
	const static cmd_t app_cmd = { .name = "dump", .run = cmd_dump, .info = cmd_dumpInfo };

	cmd_reg(&app_cmd);
}
