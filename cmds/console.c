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
	lib_printf("sets console to device, optionally adding up to %d mirror consoles, usage: console <major.minor> [<major.minor> ...]", CONSOLE_MIRRORS);
}


static int cmd_consoleParse(char *s, unsigned int *major, unsigned int *minor)
{
	char *endptr;

	*major = lib_strtoul(s, &endptr, 0);
	if (*endptr != '.') {
		log_error("\nWrong major value: %s", s);
		return -1;
	}

	*minor = lib_strtoul(++endptr, &endptr, 0);
	if (*endptr != '\0') {
		log_error("\nWrong minor value: %s", s);
		return -1;
	}
	return 0;
}


static int cmd_console(int argc, char *argv[])
{
	unsigned int major, minor;
	unsigned int mirrorMajors[CONSOLE_MIRRORS], mirrorMinors[CONSOLE_MIRRORS];
	int i;

	if (argc == 1) {
		log_error("\n%s: Arguments have to be defined", argv[0]);
		return CMD_EXIT_FAILURE;
	}
	else if (argc > (2 + CONSOLE_MIRRORS)) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}


	if (cmd_consoleParse(argv[1], &major, &minor) < 0) {
		return CMD_EXIT_FAILURE;
	}

	for (i = 0; (i + 2) < argc; i++) {
		if (cmd_consoleParse(argv[i + 2], &mirrorMajors[i], &mirrorMinors[i]) < 0) {
			return CMD_EXIT_FAILURE;
		}
	}

	lib_printf("\nconsole: Setting console to %u.%u", major, minor);
	if (argc > 2) {
		lib_printf("\nconsole: Setting console mirrors to %d.%d", mirrorMajors[0], mirrorMinors[0]);
		for (i = 1; (i + 2) < argc; i++) {
			lib_printf(", %d.%d", mirrorMajors[i], mirrorMinors[i]);
		}
	}
	/*
	 * Always set mirrors to prevent accidental mirroring to itself.
	 * console 0.0 3.0
	 * console 3.0
	 */
	lib_consoleSetMirrors(argc - 2, mirrorMajors, mirrorMinors);
	lib_consoleSet(major, minor);
	return CMD_EXIT_SUCCESS;
}


static const cmd_t console_cmd __attribute__((section("commands"), used)) = {
	.name = "console", .run = cmd_console, .info = cmd_consoleInfo
};
