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

#include <hal/hal.h>
#include <lib/log.h>
#include <lib/console.h>


static void cmd_consoleInfo(void)
{
	lib_printf("sets console to device, usage: console <major.minor>");
}


static int cmd_console(char *s)
{
	char *endptr;
	unsigned int major, minor, argsc = 0;
	char (*args)[SIZE_CMD_ARG_LINE];

	argsc = cmd_getArgs(s, ". \t", &args);
	if (argsc == 0) {
		log_error("\nArguments have to be defined");
		return -EINVAL;
	}
	else if (argsc != 2) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	/* Get major/minor */
	major = lib_strtoul(args[0], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong major value: %s", args[0]);
		return -EINVAL;
	}

	minor = lib_strtoul(args[1], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong minor value: %s", args[1]);
		return -EINVAL;
	}

	console_set(major, minor);
	return EOK;
}


__attribute__((constructor)) static void cmd_consoleReg(void)
{
	const static cmd_t app_cmd = { .name = "console", .run = cmd_console, .info = cmd_consoleInfo };
	cmd_reg(&app_cmd);
}
