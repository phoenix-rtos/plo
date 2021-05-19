/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * script command
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

/* Linker symbol points to the beginning of .data section */
extern char script[];


static void cmd_scriptInfo(void)
{
	lib_printf("shows script, usage: script <pre-init/name>");
}


/* TODO: print other scripts than pre-init and parse args */
static int cmd_script(char *s)
{
	unsigned int argsc;
	char (*args)[SIZE_CMD_ARG_LINE];

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);

	if (argsc != 1) {
		log_error("\nWrong args: %s", s);
		return ERR_NONE;
	}

	if (hal_strcmp(args[0], "pre-init") == 0) {
		lib_printf("\n%s", (char *)script);
	}
	else {
		log_error("\nScript %s does not exist", args[0]);
		return ERR_NONE;
	}

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_scriptReg(void)
{
	const static cmd_t app_cmd = { .name = "script", .run = cmd_script, .info = cmd_scriptInfo };
	cmd_reg(&app_cmd);
}
