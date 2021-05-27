/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Call command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <phfs/phfs.h>


static void cmd_callInfo(void)
{
	lib_printf("calls user's script, usage: call <dev> <script name> <magic>");
}


static int cmd_call(char *s)
{
	char c;
	int res, i;
	handler_t h;
	addr_t offs = 0;
	unsigned int argsc;
	char buff[SIZE_CMD_ARG_LINE];
	cmdarg_t *args;

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	if (argsc == 0) {
		log_error("\nArguments have to be defined");
		return -EINVAL;
	}
	else if (argsc != 3) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	/* ARG_0: device name - args[0]
	 * ARG_1: script name - args[1] */
	if (phfs_open(args[0], args[1], 0, &h) < 0) {
		log_error("\nCan't open %s, on %s", args[1], args[0]);
		return -EINVAL;
	}

	/* ARG_2: magic number*/
	if ((res = phfs_read(h, offs, (u8 *)buff, SIZE_MAGIC_NB)) < 0) {
		log_error("\nCan't read %s from %s", args[1], args[0]);
		return -EIO;
	}
	offs += res;
	buff[res] = '\0';

	/* Check magic number */
	if (hal_strcmp(buff, args[2]) != 0) {
		log_error("\nMagic number for %s is wrong.", args[1]);
		return -EINVAL;
	}

	/* Execute script */
	i = 0;
	lib_printf(CONSOLE_NORMAL);
	do {
		if ((res = phfs_read(h, offs, (u8 *)&c, 1)) < 0) {
			log_error("\nCan't read %s from %s", args[1], args[0]);
			return -EIO;
		}

		offs += res;
		if (res == 0 || c == '\n' || i == (sizeof(buff) - 1)) {
			buff[i] = '\0';
			cmd_parse(buff);
			i = 0;
			continue;
		}
		buff[i++] = c;
	} while (res > 0);

	return EOK;
}


__attribute__((constructor)) static void cmd_callReg(void)
{
	const static cmd_t app_cmd = { .name = "call", .run = cmd_call, .info = cmd_callInfo };
	cmd_reg(&app_cmd);
}
