/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Console command
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
#include "log.h"
#include "console.h"


static void cmd_consoleInfo(void)
{
	lib_printf("sets console to device, usage: console <major.minor>");
}


static int cmd_console(char *s)
{
	u16 argsc;
	char *endptr;
	unsigned int major, minor, pos = 0;
	char args[3][SIZE_CMD_ARG_LINE + 1];

	for (argsc = 0; argsc < 3; ++argsc) {
		if (cmd_getnext(s, &pos, ". \t", NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	if (argsc != 2) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	/* Get major/minor */
	major = lib_strtoul(args[0], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong major value: %s", args[0]);
		return ERR_ARG;
	}

	minor = lib_strtoul(args[1], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong minor value: %s", args[1]);
		return ERR_ARG;
	}

	console_set(major, minor);
	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_consoleReg(void)
{
	const static cmd_t app_cmd = { .name = "console", .run = cmd_console, .info = cmd_consoleInfo };
	cmd_reg(&app_cmd);
}
