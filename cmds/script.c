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
#include "hal.h"
#include "../errors.h"
#include "../plostd.h"


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

		if (plostd_ishex(alias[i]) < 0)
			return ERR_ARG;

		if (i == 1)
			addr = plostd_ahtoi(alias[i]);
		else if (i == 2)
			sz = plostd_ahtoi(alias[i]);
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
			plostd_printf(ATTR_ERROR, "\nWarning: too many script lines, only %d loaded.\n", MAX_SCRIPT_LINES_NB);
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
		plostd_printf(ATTR_INIT, "\n%s", script_common.cmds[i]);
		cmd_parse(script_common.cmds[i]);
	}
}
