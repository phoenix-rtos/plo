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

#include <lib/lib.h>
#include <lib/log.h>
#include <lib/types.h>
#include <lib/errno.h>


#define DEFAULT_BLANKS " \t"
#define DEFAULT_CITES  "\""

#define SIZE_CMD_ARG_LINE 81
#define SIZE_MSG_BUFF     0x100
#define SIZE_MAGIC_NB     8

#define MAX_CMD_ARGS_NB 10


typedef struct {
	const char name[12];
	const int (*run)(char *);
	const void (*info)(void);
} cmd_t;


/* Function registers new command */
extern void cmd_reg(const cmd_t *cmd);


/* Function runs pre-init script */
extern void cmd_run(void);


/* Function shows prompt and start interaction with user */
extern void cmd_prompt(void);


/* Function returns command specified by id */
extern const cmd_t *cmd_getCmd(unsigned int id);


/* Function parses loader commands */
extern void cmd_parse(char *line);


/* Function prase arguments from command */
extern int cmd_getArgs(const char *cmd, const char *blank, char (**args)[SIZE_CMD_ARG_LINE]);


#endif
