/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Print help message
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"


static void cmd_helpInfo(void)
{
	lib_printf("prints this message");
}


static int cmd_help(int argc, char *argv[])
{
	const cmd_t *cmd;
	unsigned int i = 0;

	if (argc != 1) {
		log_error("\n%s: Command does not accept arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	while ((cmd = cmd_getCmd(i++)) != NULL) {
		lib_printf("\n  %-12s - ", cmd->name);
		cmd->info();
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t help_cmd __attribute__((section("commands"), used)) = {
	.name = "help", .run = cmd_help, .info = cmd_helpInfo
};
