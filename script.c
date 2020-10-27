/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * comamnds script parser
 *
 * Copyright 2020 Phoenix Systems
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "script.h"
#include "plostd.h"
#include "cmd.h"



/* Linker symbol points to the beginning of .data section */
extern char script[];


struct {
	u32 cmdCnt;
	char basicCmds[MAX_COMMANDS_NB][LINESZ];
} script_common;


void script_init(void)
{
	int i;
	unsigned int pos = 0;
	char *argsScript = (char *)script;

	script_common.cmdCnt = 0;

	for (i = 0; script_common.cmdCnt < MAX_COMMANDS_NB; ++i) {
		cmd_skipblanks(argsScript, &pos, "\n");
		if (cmd_getnext(argsScript, &pos, "\n", NULL, script_common.basicCmds[i], sizeof(script_common.basicCmds[i])) == NULL || (script_common.basicCmds[i][0] == '\0'))
			break;
		script_common.cmdCnt++;
	}
}


void script_run(void)
{
	int i;

	for (i = 0; i < script_common.cmdCnt; ++i) {
		if (script_common.basicCmds[i][0] != '@') {
			plostd_printf(ATTR_INIT, "\n%s", script_common.basicCmds[i]);
			cmd_parse(script_common.basicCmds[i]);
		}
	}
}


int script_expandAlias(char **name)
{
	int i;

	for (i = 0; i < script_common.cmdCnt; ++i) {
		if (script_common.basicCmds[i][0] == '@' && plostd_strncmp(*name + 1, script_common.basicCmds[i] + 1, plostd_strlen(*name) - 1) == 0) {
			*name = script_common.basicCmds[i] + 1;
			return 0;
		}
	}

	return -1;
}

