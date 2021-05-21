/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * script command
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

/* Linker symbol points to the beginning of .data section */
extern char script[];


static void cmd_scriptInfo(void)
{
	lib_printf("shows script, usage: script <preinit/name> [dev] [magic]");
}


static int cmd_script(char *s)
{
	int res;
	handler_t h;
	addr_t offs = 0;
	unsigned int argsc;
	char buff[SIZE_CMD_ARG_LINE];
	char (*args)[SIZE_CMD_ARG_LINE];

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);

	if (argsc == 0) {
		log_error("\nArguments have to be defined");
		return -EINVAL;
	}
	else if (argsc != 1 && argsc != 3) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	if (hal_strcmp(args[0], "preinit") == 0) {
		lib_printf("\n%s", (char *)script);
		return EOK;
	}

	/* Show user script */
	if (argsc != 3) {
		log_error("\nProvide device and magic number for %s", args[0]);
		return EOK;
	}

	if (phfs_open(args[0], args[1], 0, &h) < 0) {
		log_error("\nCan't open %s, on %s", args[1], args[0]);
		return -EINVAL;
	}

	hal_memset(buff, 0, SIZE_CMD_ARG_LINE);
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

	lib_printf("\n");
	do {
		if ((res = phfs_read(h, offs, (u8 *)buff, SIZE_CMD_ARG_LINE)) < 0) {
			log_error("\nCan't read %s from %s", args[1], args[0]);
			return -EINVAL;
		}
		buff[res] = '\0';
		lib_printf(buff);
		offs += res;
	} while (res > 0);

	return EOK;
}


__attribute__((constructor)) static void cmd_scriptReg(void)
{
	const static cmd_t app_cmd = { .name = "script", .run = cmd_script, .info = cmd_scriptInfo };
	cmd_reg(&app_cmd);
}
