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
#include "log.h"
#include "hal.h"


static void cmd_echoInfo(void)
{
	lib_printf("command switch on/off information logs, usage: echo <on/off>");
}


static int cmd_echo(char *s)
{
	u16 argsc;
	unsigned int pos = 0;
	char args[2][SIZE_CMD_ARG_LINE + 1];

	for (argsc = 0; argsc < 2; ++argsc) {
		if (cmd_getnext(s, &pos, DEFAULT_BLANKS, NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	/* Show echo status */
	if (argsc == 0) {
		if (log_getEcho()) {
			log_info("\nEcho is ON");
		}
		else {
			log_info("\nEcho is OFF");
		}

		return ERR_NONE;
	}

	if (argsc != 1) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	/* Set echo */
	if (hal_strcmp(args[1], "ON") || hal_strcmp(args[1], "on")) {
		log_setEcho(1);
	}
	else if (hal_strcmp(args[1], "OFF") || hal_strcmp(args[1], "off")) {
		log_info("\nOFF");
	}
	else {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_echoReg(void)
{
	const static cmd_t app_cmd = { .name = "echo", .run = cmd_echo, .info = cmd_echoInfo };
	cmd_reg(&app_cmd);
}
