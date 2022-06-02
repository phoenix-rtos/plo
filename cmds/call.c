/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Call loader script
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


static int cmd_call(int argc, char *argv[])
{
	char c;
	ssize_t len;
	int i, res;
	handler_t h;
	addr_t offs = 0;
	char buff[SIZE_CMD_ARG_LINE];

	if (argc == 1) {
		log_error("\n%s: Arguments have to be defined", argv[0]);
		return -EINVAL;
	}
	else if (argc != 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	/* ARG_0: device name - argv[1]
	 * ARG_1: script name - argv[2] */
	if ((res = phfs_open(argv[1], argv[2], 0, &h)) < 0) {
		log_error("\nCan't open %s, on %s", argv[2], argv[1]);
		return res;
	}

	/* ARG_2: magic number*/
	if ((len = phfs_read(h, offs, buff, SIZE_MAGIC_NB)) < 0) {
		log_error("\nCan't read %s from %s", argv[2], argv[1]);
		return len;
	}
	offs += len;
	buff[len] = '\0';

	/* Check magic number, don't return error, as there might be a next script */
	if (hal_strcmp(buff, argv[3]) != 0) {
		log_error("\nMagic number for %s is wrong.", argv[2]);
		return EOK;
	}

	/* Execute script */
	i = 0;
	lib_printf(CONSOLE_NORMAL);
	do {
		if ((len = phfs_read(h, offs, &c, 1)) < 0) {
			log_error("\nCan't read %s from %s", argv[2], argv[1]);
			return len;
		}

		offs += len;
		if (len == 0 || c == '\n' || i == (sizeof(buff) - 1)) {
			buff[i] = '\0';
			if ((res = cmd_parse(buff)) < 0)
				return res;
			i = 0;
			continue;
		}
		buff[i++] = c;
	} while (len > 0);

	return EOK;
}


__attribute__((constructor)) static void cmd_callReg(void)
{
	const static cmd_t app_cmd = { .name = "call", .run = cmd_call, .info = cmd_callInfo };
	cmd_reg(&app_cmd);
}
