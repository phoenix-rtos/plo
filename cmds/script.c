/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Print loader script
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
#include <phfs/phfs.h>

/* Linker symbol points to the beginning of .data section */
extern char script[];


static void cmd_scriptInfo(void)
{
	lib_printf("shows script, usage: script [<dev> <name> <magic>]");
}


static int cmd_script(int argc, char *argv[])
{
	ssize_t res;
	handler_t h;
	addr_t offs = 0;
	char buff[SIZE_CMD_ARG_LINE];

	if (argc == 1) {
		lib_printf(CONSOLE_BOLD "\nPreinit script:");
		lib_printf(CONSOLE_NORMAL "\n%s", (char *)script);
		return CMD_EXIT_SUCCESS;
	}

	if (argc != 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	res = phfs_open(argv[1], argv[2], 0, &h);
	if (res < 0) {
		log_error("\nCan't open %s, on %s (%d)", argv[2], argv[1], res);
		return CMD_EXIT_FAILURE;
	}

	res = phfs_read(h, offs, buff, SIZE_MAGIC_NB);
	if (res < 0) {
		log_error("\nCan't read %s from %s (%d)", argv[2], argv[1], res);
		phfs_close(h);
		return CMD_EXIT_FAILURE;
	}
	offs += res;
	buff[res] = '\0';

	/* Check magic number */
	if (hal_strcmp(buff, argv[3]) != 0) {
		log_error("\nMagic number for %s is wrong.", argv[2]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}

	lib_printf(CONSOLE_BOLD "\nScript - %s:", argv[2]);
	lib_printf(CONSOLE_NORMAL);
	do {
		res = phfs_read(h, offs, buff, SIZE_CMD_ARG_LINE - 1);
		if (res < 0) {
			log_error("\nCan't read %s from %s (%d)", argv[2], argv[1], res);
			phfs_close(h);
			return CMD_EXIT_FAILURE;
		}
		buff[res] = '\0';
		lib_printf(buff);
		offs += res;
	} while (res > 0);

	phfs_close(h);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t script_cmd __attribute__((section("commands"), used)) = {
	.name = "script", .run = cmd_script, .info = cmd_scriptInfo
};
