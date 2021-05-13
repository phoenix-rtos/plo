/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * timeout command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "console.h"


static void cmd_timeoutInfo(void)
{
	lib_printf("boots timeout in seconds, usage: timeout [<timeout>]");
}


static int cmd_timeout(char *s)
{
	char c;
	u16 argsc;
	unsigned int time, pos = 0;
	char args[2][SIZE_CMD_ARG_LINE + 1];

	for (argsc = 0; argsc < 2; ++argsc) {
		if (cmd_getnext(s, &pos, ". \t", NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	if (argsc != 1) {
		lib_printf("\nWrong number of arguments!!\n");
		return ERR_ARG;
	}

	time = lib_strtoul(args[0], NULL, 10);

	lib_printf("\n");
	lib_printf(CONSOLE_NORMAL);
	for (; time; --time) {
		lib_printf("\rWaiting for input, %d [s]", time);
		if (console_getc(&c, 1000) > 0)
			return -1;
	}
	lib_printf("\rWaiting for input, %d [s]", time);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_timeoutReg(void)
{
	const static cmd_t app_cmd = { .name = "timeout", .run = cmd_timeout, .info = cmd_timeoutInfo };

	cmd_reg(&app_cmd);
}
