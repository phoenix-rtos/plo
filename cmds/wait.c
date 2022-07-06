/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Wait timer
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


static void cmd_waitInfo(void)
{
	lib_printf("waits in milliseconds or in infinite loop, usage: wait [ms]");
}


static int cmd_wait(int argc, char *argv[])
{
	int i;
	char c;
	char *endptr;
	unsigned int time, step;
	static const char prompt[] = "Waiting for input";

	lib_printf("\n%s", CONSOLE_NORMAL);
	/* User doesn't provide time, waiting in infinite loop */
	if (argc != 2) {
		while (1) {
			lib_printf("\r%*s \r%s ", sizeof(prompt) + 4, "", prompt);
			for (i = 0; i < 3; ++i) {
				lib_printf(".");
				if (lib_consoleGetc(&c, 500) > 0)
					return -1;
			}
		}
	}

	/* Wait for the time specified by the user */
	time = lib_strtoul(argv[1], &endptr, 0);
	if (*endptr) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	while (time > 0) {
		step = time >= 100 ? 100 : time;
		time -= step;
		lib_printf("\r%*s \r%s, %5d [ms]", sizeof(prompt) + 14, "", prompt, time);
		if (lib_consoleGetc(&c, step) > 0)
			return -1;
	}
	lib_printf("\r%*s \r%s, %5d [ms]\n", sizeof(prompt) + 14, "", prompt, 0);

	return EOK;
}


__attribute__((constructor)) static void cmd_waitReg(void)
{
	const static cmd_t app_cmd = { .name = "wait", .run = cmd_wait, .info = cmd_waitInfo };

	cmd_reg(&app_cmd);
}
