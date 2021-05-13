/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * syspage command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "syspage.h"


static void cmd_syspageInfo(void)
{
	lib_printf("shows syspage contents, usage: syspage");
}


static int cmd_syspage(char *s)
{
	unsigned int pos = 0;

	cmd_skipblanks(s, &pos, DEFAULT_BLANKS);
	s += pos;

	if (*s) {
		lib_printf("\nCommand syspage does not take any arguments\n");
		return ERR_ARG;
	}

	syspage_show();

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_syspageReg(void)
{
	const static cmd_t app_cmd = { .name = "syspage", .run = cmd_syspage, .info = cmd_syspageInfo };

	cmd_reg(&app_cmd);
}
