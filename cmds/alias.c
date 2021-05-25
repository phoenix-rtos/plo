/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Load application command
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
#include <phfs/phfs.h>


static void cmd_aliasInfo(void)
{
	lib_printf("sets alias to file, usage <name> <offset> <size>");
}


static int cmd_alias(char *s)
{
	char *end;
	size_t sz = 0;
	addr_t addr = 0;
	unsigned int i, argsc;
	cmdarg_t *args;

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	if (argsc == 0) {
		phfs_showFiles();
		return EOK;
	}
	else if (argsc != 3) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}


	for (i = 0; i < 3; ++i) {
		if (i == 0)
			continue;

		if (i == 1)
			addr = lib_strtoul(args[i], &end, 0);
		else if (i == 2)
			sz = lib_strtoul(args[i], &end, 0);

		if (*end) {
			log_error("\nWrong args: %s", s);
			return -EINVAL;
		}
	}

	if (phfs_regFile(args[0], addr, sz) < 0) {
		log_error("\nCan't register file %s", args[0]);
		return -EINVAL;
	}

	log_info("\nRegistering file %s ", args[0]);

	return EOK;
}


__attribute__((constructor)) static void cmd_aliasReg(void)
{
	const static cmd_t alias_cmd = { .name = "alias", .run = cmd_alias, .info = cmd_aliasInfo };
	cmd_reg(&alias_cmd);
}
