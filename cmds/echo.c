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
#include <lib/ctype.h>
#include <hal/hal.h>


static void cmd_echoInfo(void)
{
	lib_printf("command switch on/off information logs, usage: echo <on/off>");
}


static int cmd_echo(char *s)
{
	unsigned int argsc;
	cmdarg_t *args;
	char *p;

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

	/* convert to lowercase */
	p = args[0];
	for (p = args[0]; *p; p++) {
		if (isupper(*p))
			*p |= 0x20;
	}

	/* Set echo */
	if ((args[0][0] == 'o') && (args[0][1] == 'n') && (args[0][2] == '\0')) {
		log_setEcho(1);
	}
	else if ((args[0][0] == 'o') && (args[0][1] == 'f') && (args[0][2] == 'f') && (args[0][3] == '\0')) {
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
