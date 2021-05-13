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

/* Linker symbol points to the beginning of .data section */
extern char script[];


static void cmd_scriptInfo(void)
{
	lib_printf("shows script, usage: script <number>");
}


static int cmd_script(char *s)
{
	unsigned int pos = 0;

	cmd_skipblanks(s, &pos, DEFAULT_BLANKS);
	s += pos;

	if (*s) {
		lib_printf("\nCommand script does not take any arguments\n");
		return ERR_ARG;
	}

	lib_printf("\n%s", (char *)script);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_scriptReg(void)
{
	const static cmd_t app_cmd = { .name = "script", .run = cmd_script, .info = cmd_scriptInfo };
	cmd_reg(&app_cmd);
}
