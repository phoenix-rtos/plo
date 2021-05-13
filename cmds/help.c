/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * help command
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


static int cmd_help(char *s)
{
	const cmd_t *cmd;
	unsigned int i = 0;

	lib_printf("\n");
	while ((cmd = cmd_getCmd(i++)) != NULL) {
		lib_printf("  %-12s - ", cmd->name);
		cmd->info();
		lib_printf("\n");
	}

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_helpReg(void)
{
	const static cmd_t app_cmd = { .name = "help", .run = cmd_help, .info = cmd_helpInfo };

	cmd_reg(&app_cmd);
}
