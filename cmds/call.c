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
	res = phfs_open(argv[1], argv[2], 0, &h);
	if (res < 0) {
		log_error("\nCan't open %s, on %s", argv[2], argv[1]);
		return res;
	}

	/* ARG_2: magic number*/
	len = phfs_read(h, offs, buff, SIZE_MAGIC_NB);
	if (len != SIZE_MAGIC_NB) {
		log_error("\nCan't read %s from %s", argv[2], argv[1]);
		phfs_close(h);
		return (len < 0) ? len : -EIO;
	}
	offs += len;
	buff[len] = '\0';

	/* Check magic number, don't return error, as there might be a next script */
	if (hal_strcmp(buff, argv[3]) != 0) {
		log_error("\nMagic number for %s is wrong.", argv[2]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}

	/* Execute script */
	i = 0;
	lib_printf(CONSOLE_NORMAL);
	do {
		len = phfs_read(h, offs, &c, 1);
		if (len < 0) {
			log_error("\nCan't read %s from %s", argv[2], argv[1]);
			phfs_close(h);
			return len;
		}

		offs += len;
		if ((len == 0) || (c == '\n') || (c == '\0' /* EOF */)) {
			buff[i] = '\0';
			res = cmd_parse(buff);
			if (res != CMD_EXIT_SUCCESS) {
				return (res < 0) ? res : -EINVAL;
			}
			i = 0;
			continue;
		}

		if (i == (sizeof(buff) - 1)) {
			log_error("\nLine in %s exceeds buffer size", argv[2]);
			phfs_close(h);
			return -ENOMEM;
		}

		buff[i++] = c;
	} while (len > 0);

	phfs_close(h);
	return CMD_EXIT_SUCCESS;
}


static const cmd_t call_cmd __attribute__((section("commands"), used)) = {
	.name = "call", .run = cmd_call, .info = cmd_callInfo
};
