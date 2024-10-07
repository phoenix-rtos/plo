/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Set console device
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>


static void cmd_consoleInfo(void)
{
	lib_printf("sets console to device, usage: console <major.minor>");
}


static int cmd_console(int argc, char *argv[])
{
	char *endptr;
	unsigned int major, minor;

	if (argc == 1) {
		log_error("\n%s: Arguments have to be defined", argv[0]);
		return CMD_EXIT_FAILURE;
	}
	else if (argc != 2) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}


	/* Get major/minor */
	major = lib_strtoul(argv[1], &endptr, 0);
	if (*endptr != '.') {
		log_error("\nWrong major value: %s", argv[1]);
		return CMD_EXIT_FAILURE;
	}

	minor = lib_strtoul(++endptr, &endptr, 0);
	if (*endptr != '\0') {
		log_error("\nWrong minor value: %s", argv[1]);
		return CMD_EXIT_FAILURE;
	}

	lib_printf("\nconsole: Setting console to %d.%d", major, minor);
	lib_consoleSet(major, minor);
	return CMD_EXIT_SUCCESS;
}


static const cmd_t console_cmd __attribute__((section("commands"), used)) = {
	.name = "console", .run = cmd_console, .info = cmd_consoleInfo
};
