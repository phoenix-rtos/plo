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
#include "phfs.h"
#include "cmd.h"
#include "lib.h"
#include "errors.h"


/* Linker symbol points to the beginning of .data section */
extern char script[];


struct {
	u32 cnt;
	char cmds[MAX_SCRIPT_LINES_NB][LINESZ];
} script_common;


static int script_regFile(char *cmd)
{
	int i;
	size_t sz = 0;
	addr_t addr = 0;
	unsigned int pos = 0;
	char alias[3][LINESZ + 1];

	for (i = 0; i < 3; ++i) {
		if (cmd_getnext(cmd, &pos, "(:) \t", NULL, alias[i], sizeof(alias[i])) == NULL || *alias[i] == 0)
			break;

		if (i == 0)
			continue;

		if (lib_ishex(alias[i]) < 0)
			return ERR_ARG;

		if (i == 1)
			addr = lib_strtoul(alias[i], NULL, 16);
		else if (i == 2)
			sz = lib_strtoul(alias[i], NULL, 16);
	}

	if (phfs_regFile(alias[0] + 1, addr, sz) < 0)
		return ERR_ARG;

	return ERR_NONE;
}


void script_init(void)
{
	int i;
	unsigned int pos = 0;
	char *argsScript = (char *)script;

	for (i = 0;; ++i) {
		if (i >= MAX_SCRIPT_LINES_NB) {
			lib_printf("\nWarning: too many script lines, only %d loaded.\n", MAX_SCRIPT_LINES_NB);
			break;
		}

		cmd_skipblanks(argsScript, &pos, "\n");
		if (cmd_getnext(argsScript, &pos, "\n", NULL, script_common.cmds[script_common.cnt], sizeof(script_common.cmds[script_common.cnt])) == NULL || (script_common.cmds[script_common.cnt][0] == '\0'))
			break;

		/* Alliases to files are not registered as commands */
		if (script_common.cmds[script_common.cnt][0] == '@') {
			script_regFile(script_common.cmds[script_common.cnt]);
			continue;
		}

		script_common.cnt++;
	}
}


void script_run(void)
{
	int i;

	for (i = 0; i < script_common.cnt; ++i) {
		lib_printf("\n%s", script_common.cmds[i]);
		cmd_parse(script_common.cmds[i]);
	}
}
