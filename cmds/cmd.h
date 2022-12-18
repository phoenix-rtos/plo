/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Loader commands
 *
 * Copyright 2012, 2020 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CMD_H_
#define _CMD_H_

#include <lib/lib.h>


#define SIZE_CMD_ARG_LINE 256
#define SIZE_MSG_BUFF     0x100
#define SIZE_MAGIC_NB     8

/* Reserve +1 for terminating NULL pointer in conformance to C standard */
#define SIZE_CMD_ARGV (10 + 1)

/* Command exit statuses */
#define CMD_EXIT_FAILURE 1
#define CMD_EXIT_SUCCESS 0


typedef struct {
	const char name[12];
	int (*const run)(int, char *[]);
	void (*const info)(void);
} cmd_t;


/* Function registers new command */
extern void cmd_reg(const cmd_t *cmd);


/* Function runs pre-init script */
extern int cmd_run(void);


/* Function shows prompt and start interaction with user */
extern void cmd_prompt(void);


/* Function returns command specified by id */
extern const cmd_t *cmd_getCmd(unsigned int id);


/* Function parses loader commands */
extern int cmd_parse(const char *line);


#endif
