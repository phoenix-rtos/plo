/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * devs command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "phfs.h"


static void cmd_devsInfo(void)
{
	lib_printf("shows registered devs in phfs, usage: devs");
}


static int cmd_devs(char *s)
{
	unsigned int pos = 0;

	cmd_skipblanks(s, &pos, DEFAULT_BLANKS);
	s += pos;

	if (*s) {
		lib_printf("\nCommand devs does not take any arguments\n");
		return ERR_ARG;
	}

	phfs_showDevs();

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_devsReg(void)
{
	const static cmd_t app_cmd = { .name = "devs", .run = cmd_devs, .info = cmd_devsInfo };

	cmd_reg(&app_cmd);
}
