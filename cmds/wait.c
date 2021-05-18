/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * wait command
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
#include "console.h"


static void cmd_waitInfo(void)
{
	lib_printf("waits in milliseconds or in infinite loop, usage: wait [ms]");
}


static int cmd_wait(char *s)
{
	int i;
	char c;
	u16 argsc;
	char *endptr;
	unsigned int time, step, pos = 0;
	char args[2][SIZE_CMD_ARG_LINE + 1];
	static const char prompt[] = "Waiting for input";

	for (argsc = 0; argsc < 2; ++argsc) {
		if (cmd_getnext(s, &pos, ". \t", NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	lib_printf("\n%s %s", CONSOLE_NORMAL);
	/* User doesn't provide time, waiting in infinite loop */
	if (argsc != 1) {
		while(1) {
			lib_printf("\r%*s \r%s ", sizeof(prompt) + 4, "", prompt);
			for (i = 0; i < 3; ++i) {
				lib_printf(".");
				if (console_getc(&c, 500) > 0)
					return -1;
			}
		}
	}

	/* Wait for the time specified by the user */
	time = lib_strtoul(args[0], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	while (time > 0) {
		step = time >= 100 ? 100 : time;
		time -= step;
		lib_printf("\r%*s \r%s, %5d [ms]", sizeof(prompt) + 14, "", prompt, time);
		if (console_getc(&c, step) > 0)
			return -1;
	}
	lib_printf("\r%*s \r%s, %5d [ms]", sizeof(prompt) + 14, "", prompt, 0);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_waitReg(void)
{
	const static cmd_t app_cmd = { .name = "wait", .run = cmd_wait, .info = cmd_waitInfo };

	cmd_reg(&app_cmd);
}
