/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * End script execution
 *
 * Copyright 2024 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>


static void cmd_stopInfo(void)
{
	lib_printf("End script execution, usage: stop [message]");
}


static int cmd_stop(int argc, char *argv[])
{
	int i;

	lib_printf("\nScript stopped");

	if (argc != 1) {
		lib_printf(":");
	}

	for (i = 1; i < argc; ++i) {
		lib_printf(" %s", argv[i]);
	}

	/* Break script execution by simulating an error */
	return CMD_EXIT_FAILURE;
}


static cmd_t stop_cmd __attribute__((section("commands"), used)) = {
	.name = "stop", .run = cmd_stop, .info = cmd_stopInfo
};
