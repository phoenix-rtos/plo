/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Generic loader commands
 *
 * Copyright 2012, 2020 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CMD_H_
#define _CMD_H_

#include "types.h"


/* Boot command size */
#define CMD_SIZE          64


#define DEFAULT_BLANKS    " \t"
#define DEFAULT_CITES     "\""


/* Function parses loader commands */
extern void cmd_parse(char *line);


/* Generic command handlers */
extern void cmd_help(char *s);

extern void cmd_timeout(char *s);

extern void cmd_go(char *s);

extern void cmd_cmd(char *s);


/* Specific command handlers - defining per platform */
extern void cmd_dump(char *s);

extern void cmd_load(char *s);

extern void cmd_memmap(char *s);

extern void cmd_copy(char *s);

extern void cmd_save(char *s);

extern void cmd_lspci(char *s);


/* Auxiliary functions */
extern void cmd_showprogress(u32 p);

extern void cmd_skipblanks(char *line, unsigned int *pos, char *blanks);

extern char *cmd_getnext(char *line, unsigned int *pos, char *blanks, char *cites, char *word, unsigned int len);

#endif
