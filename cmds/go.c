/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * go command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/log.h>
#include <lib/console.h>


static void cmd_goInfo(void)
{
	lib_printf("starts Phoenix-RTOS loaded into memory");
}


static int cmd_go(char *s)
{
	unsigned int argsc;
	cmdarg_t *args;

	if ((argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args)) != 0) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	log_info("\nRunning Phoenix-RTOS");
	lib_printf(CONSOLE_NORMAL CONSOLE_CLEAR CONSOLE_CURSOR_SHOW);
	hal_launch();

	return EOK;
}


__attribute__((constructor)) static void cmd_goReg(void)
{
	const static cmd_t app_cmd = { .name = "go!", .run = cmd_go, .info = cmd_goInfo };

	cmd_reg(&app_cmd);
}
