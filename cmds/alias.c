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
#include "hal.h"
#include "phfs.h"


static void cmd_aliasInfo(void)
{
	lib_printf("sets alias to file, usage <name> <offset> <size>");
}


static int cmd_alias(char *s)
{
	char *end;
	size_t sz = 0;
	addr_t addr = 0;
	unsigned int i, pos = 0;
	char alias[3][SIZE_CMD_ARG_LINE + 1];

	for (i = 0; i < 3; ++i) {
		if (cmd_getnext(s, &pos, DEFAULT_BLANKS, NULL, alias[i], sizeof(alias[i])) == NULL || *alias[i] == 0)
			break;

		if (i == 0)
			continue;

		if (i == 1)
			addr = lib_strtoul(alias[i], &end, 0);
		else if (i == 2)
			sz = lib_strtoul(alias[i], &end, 0);

		if (hal_strlen(end) != 0) {
			log_error("\nWrong args: %s", s);
			return ERR_ARG;
		}
	}

	if (phfs_regFile(alias[0], addr, sz) < 0) {
		log_error("\nCan't register file %s", alias[0]);
		return ERR_ARG;
	}

	log_info("\nRegistering file %s ", alias[0]);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_aliasReg(void)
{
	const static cmd_t alias_cmd = { .name = "alias", .run = cmd_alias, .info = cmd_aliasInfo };
	cmd_reg(&alias_cmd);
}
