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


static int cmd_script(int argc, char *argv[])
{
	ssize_t res;
	handler_t h;
	addr_t offs = 0;
	char buff[SIZE_CMD_ARG_LINE];

	if (argc == 1) {
		log_error("\n%s: Arguments have to be defined", argv[0]);
		return -EINVAL;
	}
	else if (argc != 2 && argc != 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	if (hal_strcmp(argv[1], "preinit") == 0) {
		lib_printf("\n%s", (char *)script);
		return EOK;
	}

	/* Show user script */
	if (argc != 4) {
		log_error("\nProvide device and magic number for %s", argv[1]);
		return EOK;
	}

	if ((res = phfs_open(argv[1], argv[2], 0, &h)) < 0) {
		log_error("\nCan't open %s, on %s", argv[2], argv[1]);
		return res;
	}

	if ((res = phfs_read(h, offs, (u8 *)buff, SIZE_MAGIC_NB)) < 0) {
		log_error("\nCan't read %s from %s", argv[2], argv[1]);
		return res;
	}
	offs += res;
	buff[res] = '\0';

	/* Check magic number */
	if (hal_strcmp(buff, argv[3]) != 0) {
		log_error("\nMagic number for %s is wrong.", argv[2]);
		return -EINVAL;
	}

	lib_printf("\n");
	do {
		if ((res = phfs_read(h, offs, (u8 *)buff, SIZE_CMD_ARG_LINE)) < 0) {
			log_error("\nCan't read %s from %s", argv[2], argv[1]);
			return res;
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
