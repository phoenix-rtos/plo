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
#include "hal.h"


/* Linker symbol points to the beginning of .data section */
extern char script[];


struct {
	u32 cmdCnt;
	char *basicCmds[MAX_SCRIPT_LINES_NB];
} script_common;


void script_init(void)
{
	unsigned int i, len;
	char *s = script;

	script_common.cmdCnt = 0;

	for (i = 0; *s; ++i) {
		if (i >= MAX_SCRIPT_LINES_NB) {
			plostd_printf(ATTR_ERROR, "\nWarning: too many script lines, only %d loaded.\n", MAX_SCRIPT_LINES_NB);
			break;
		}

		while (*s == '\n')
			s++;
		if (*s == '\0')
			break;

		len = 0;
		script_common.basicCmds[i] = s;
		while (*s && (*s != '\n')) {
			len++;
			s++;
		}
		*s++ = '\0';

		if (len >= LINESZ) {
			plostd_printf(ATTR_ERROR, "\nWarning: script line to long:\n%s\n", script_common.basicCmds[i]);
			break;
		}

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


int script_expandAlias(char *name)
{
	unsigned int i, len, pos;

	/* Computing the length of the name and positions of the arguments */
	len = plostd_strlen(name);
	if (len == 1)
		return -1;

	pos = 1;
	while (pos < len) {
		if (name[pos] == ';')
			break;
		++pos;
	}
	--pos;

	for (i = 0; i < script_common.cmdCnt; ++i) {
		if (script_common.basicCmds[i][0] == '@') {
			if (plostd_strncmp(name + 1, script_common.basicCmds[i] + 1, pos) == 0) {
				/* Copy size and offset to app name */
				hal_memcpy(name + len, script_common.basicCmds[i] + pos + 1, plostd_strlen(script_common.basicCmds[i]) - pos);
				return 0;
			}
		}
	}

	return -1;
}
