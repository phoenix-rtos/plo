/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Echo command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <lib/log.h>
#include <hal/hal.h>


static void cmd_echoInfo(void)
{
	lib_printf("command switch on/off information logs, usage: echo [on/off]");
}


static int cmd_echo(char *s)
{
	unsigned int argsc;
	cmdarg_t *args;

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	/* Show echo status */
	if (argsc == 0) {
		if (log_getEcho()) {
			log_info("\nEcho is ON");
		}
		else {
			log_info("\nEcho is OFF");
		}

		return EOK;
	}

	if (argsc != 1) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	/* Set echo */
	if (hal_strcmp(args[0], "ON") == 0 || hal_strcmp(args[0], "on") == 0) {
		log_setEcho(1);
	}
	else if (hal_strcmp(args[0], "OFF") == 0 || hal_strcmp(args[0], "off") == 0) {
		log_setEcho(0);
	}
	else {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	return EOK;
}


__attribute__((constructor)) static void cmd_echoReg(void)
{
	const static cmd_t app_cmd = { .name = "echo", .run = cmd_echo, .info = cmd_echoInfo };
	cmd_reg(&app_cmd);
}
