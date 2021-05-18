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
#include "hal.h"
#include "phfs.h"


static void cmd_callInfo(void)
{
	lib_printf("calls user's script, usage: call <dev> <script name> <magic>");
}


static int cmd_call(char *s)
{
	char c;
	int res, i;
	u16 argsc;
	handler_t h;
	addr_t offs = 0;
	phfs_stat_t stat;
	char *endptr, *buff;
	unsigned int magic, pos = 0;
	char args[4][SIZE_CMD_ARG_LINE + 1];

	for (argsc = 0; argsc < 4; ++argsc) {
		if (cmd_getnext(s, &pos, DEFAULT_BLANKS, NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	if (argsc != 3) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	/* ARG_0: device name - args[0]
	 * ARG_1: script name   - args[1] */
	if (phfs_open(args[0], args[1], 0, &h) < 0) {
		log_error("\nCan't open %s, on %s", args[1], args[0]);
		return ERR_ARG;

	}

	/* ARG_2: magic number in hex */
	magic = lib_strtoul(args[2], &endptr, 16);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong magic number: %s for %s", args[2], args[1]);
		return ERR_ARG;
	}

	if (phfs_stat(h, &stat) < 0) {
		log_error("\nCan't get stat from %s on %s", args[1], args[0]);
		return ERR_ARG;
	}

	/* Use allocated memory for args[2] which are not used anymore */
	buff = args[2];
	hal_memset(buff, 0, SIZE_CMD_ARG_LINE + 1);

	/* Check magic number */
	// if ((res = phfs_read(h, offs, (u8 *)&c, 1)) < 0) {
	// 	log_error("\nCan't read %s from %s", args[1], args[0]);
	// 	return ERR_PHFS_FILE;
	// }

	/* Execute script */
	i = 0;
	do {
		if ((res = phfs_read(h, offs, (u8 *)&c, 1)) < 0) {
			log_error("\nCan't read %s from %s", args[1], args[0]);
			return ERR_PHFS_FILE;
		}

		stat.size -= res;
		if (c == '\n' || i > SIZE_CMD_ARG_LINE) {
			i = 0;
			cmd_parse(buff);
			hal_memset(buff, 0, SIZE_CMD_ARG_LINE + 1);
			continue;
		}

		buff[i++] = c;
	} while (stat.size > 0 && res > 0);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_callReg(void)
{
	const static cmd_t app_cmd = { .name = "call", .run = cmd_call, .info = cmd_callInfo };
	cmd_reg(&app_cmd);
}
