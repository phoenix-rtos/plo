/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Loader commands
 *
 * Copyright 2012, 2017, 2020-2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Pawel Kolodziej, Hubert Buczynski, Gerard Swiderski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>


#define SIZE_HIST 8
#define SIZE_CMDS 24

#define PROMPT "(plo)% "


/* Linker symbol points to the beginning of .data section */
extern char script[];

struct {
	const cmd_t *cmds[SIZE_CMDS];
	int ll;
	int cl;
	char lines[SIZE_HIST][SIZE_CMD_ARG_LINE];
} cmd_common;


static int cmd_parseArgLine(const char **lines, char *buf, size_t bufsz, char *argv[], size_t argvsz)
{
	char *const end = buf + bufsz;
	int argc = 0;

	if (lines == NULL || *lines == NULL || **lines == '\0')
		return -1; /* EOF */

	while (**lines != '\0') {
		if (lib_isspace(**lines)) {
			if (lib_isblank(*(*lines)++))
				continue;
			else
				break;
		}

		/* Argument count and one NULL pointer */
		if (argc + 1 >= argvsz) {
			log_error("\ncmd: Too many arguments");
			return -EINVAL;
		}

		argv[argc++] = buf;

		while (lib_isgraph(**lines) && buf < end)
			*buf++ = *(*lines)++;

		if (buf >= end) {
			log_error("\ncmd: Command buffer too small");
			return -ENOMEM;
		}

		*buf++ = '\0';
	}

	argv[argc] = NULL;

	return argc;
}


void cmd_reg(const cmd_t *cmd)
{
	unsigned int i;

	for (i = 0; i < SIZE_CMDS; ++i) {
		if (cmd_common.cmds[i] != NULL) {
			if (hal_strcmp(cmd_common.cmds[i]->name, cmd->name) == 0)
				return;

			continue;
		}

		cmd_common.cmds[i] = cmd;
		break;
	}
}


int cmd_run(void)
{
	lib_printf("\ncmd: Executing pre-init script");
	return cmd_parse(script);
}


const cmd_t *cmd_getCmd(unsigned int id)
{
	if (id >= SIZE_CMDS)
		return NULL;

	return cmd_common.cmds[id];
}


int cmd_parse(const char *script)
{
	char *argv[SIZE_CMD_ARGV];
	char argline[SIZE_CMD_ARG_LINE];
	int ret = -1, argc;
	unsigned int i;

	for (;;) {
		argc = cmd_parseArgLine(&script, argline, SIZE_CMD_ARG_LINE, argv, SIZE_CMD_ARGV);

		/* skip empty lines */
		if (argc == 0)
			continue;
		/* either end of script or no script */
		else if (argc == -1)
			return EOK;
		/* error */
		else if (argc < 0)
			return argc;

		/* Find command and launch associated function */
		for (i = 0; cmd_common.cmds[i] != NULL && i < SIZE_CMDS; i++) {
			if (hal_strcmp(argv[0], cmd_common.cmds[i]->name) != 0)
				continue;

			lib_getoptReset();

			ret = cmd_common.cmds[i]->run(argc, argv);
			if (ret < 0) {
				return ret;
			}

			break;
		}

		/* If previous loop exits with ret < 0, invalid cmd is provided */
		if (ret < 0) {
			log_error("\n'%s' - unknown command!", argv[0]);
			return -EINVAL;
		}
	}
}


/* TODO: old code needs to be cleaned up */
void cmd_prompt(void)
{
	char c = 0, sc = 0;
	int pos = 0;
	int i, k, chgfl = 0, ncl;
	unsigned char newline = 0;

	cmd_common.ll = 0;
	cmd_common.cl = 0;

	for (k = 0; k < SIZE_HIST; k++)
		cmd_common.lines[k][0] = 0;

	lib_printf("\n%s", PROMPT);
	while (c != '#') {
		lib_consoleGetc(&c, -1);

		sc = 0;
		/* Translate backspace */
		if (c == 127) {
			c = 8;
		}
		/* Simple parser for VT100 commands */
		else if (c == 27) {
			lib_consoleGetc(&c, -1);

			switch (c) {
				case 91:
					lib_consoleGetc(&c, -1);

					switch (c) {
						case 'A': /* UP */
							sc = 72;
							break;
						case 'B': /* DOWN */
							sc = 80;
							break;
					}
					break;
			}
			c = 0;
		}

		/* Regular characters */
		if (c) {
			if ((c == '\r') || (c == '\n')) {
				/* handle crlf line ending */
				if (c == '\r') {
					newline = 1;
				}
				/* lf after cr - skip */
				else if ((c == '\n') && (newline != 0)) {
					newline = 0;
					continue;
				}

				if (pos) {
					cmd_parse(cmd_common.lines[cmd_common.ll]);

					cmd_common.ll = (cmd_common.ll + 1) % SIZE_HIST;
					cmd_common.cl = cmd_common.ll;
				}
				pos = 0;
				lib_printf("\n%s", PROMPT);
				continue;
			}
			else if (c == '\t') {
				/* skip */
				continue;
			}
			else {
				newline = 0;
			}

			/* If character isn't backspace add it to line buffer */
			if ((c != 8) && (pos < SIZE_CMD_ARG_LINE - 1)) {
				lib_consolePutc(c);
				cmd_common.lines[cmd_common.ll][pos++] = c;
				cmd_common.lines[cmd_common.ll][pos] = 0;
			}

			/* Remove character before cursor */
			if ((c == 8) && (pos > 0)) {
				cmd_common.lines[cmd_common.ll][--pos] = 0;
				lib_printf("%c %c", 8, 8);
			}
		}
		/* Control characters */
		else {
			switch (sc) {
				case 72:
					ncl = ((cmd_common.cl + SIZE_HIST - 1) % SIZE_HIST);
					if ((ncl != cmd_common.ll) && (cmd_common.lines[ncl][0])) {
						cmd_common.cl = ncl;
						hal_memcpy(cmd_common.lines[cmd_common.ll], cmd_common.lines[cmd_common.cl], hal_strlen(cmd_common.lines[cmd_common.cl]) + 1);
						chgfl = 1;
					}
					break;

				case 80:
					ncl = ((cmd_common.cl + 1) % SIZE_HIST);
					if (cmd_common.cl != cmd_common.ll) {
						cmd_common.cl = ncl;
						chgfl = 1;

						if (ncl != cmd_common.ll)
							hal_memcpy(cmd_common.lines[cmd_common.ll], cmd_common.lines[cmd_common.cl], hal_strlen(cmd_common.lines[cmd_common.cl]) + 1);
						else
							cmd_common.lines[cmd_common.ll][0] = 0;
					}
					break;
			}

			if (chgfl) {
				lib_printf("\r%s", PROMPT);
				for (i = 0; i < pos; i++)
					lib_printf(" ");

				pos = hal_strlen(cmd_common.lines[cmd_common.ll]);
				lib_printf("\r%s", PROMPT);
				lib_printf("%s", cmd_common.lines[cmd_common.ll]);
				chgfl = 0;
			}
		}
	}
}
