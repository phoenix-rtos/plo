/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Toggle printing loader logs
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


static int cmd_echo(int argc, char *argv[])
{
	/* Show echo status */
	if (argc == 1) {
		if (log_getEcho()) {
			lib_printf("\nEcho is 'on'");
		}
		else {
			lib_printf("\nEcho is 'off'");
		}

		return CMD_EXIT_SUCCESS;
	}

	if (argc != 2) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* Set echo */
	if (hal_strcmp(argv[1], "on") == 0) {
		log_setEcho(1);
	}
	else if (hal_strcmp(argv[1], "off") == 0) {
		log_setEcho(0);
	}
	else {
		log_error("\n%s: Wrong arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t echo_cmd __attribute__((section("commands"), used)) = {
	.name = "echo", .run = cmd_echo, .info = cmd_echoInfo
};
