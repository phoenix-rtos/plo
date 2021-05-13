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
	int i;
	size_t sz = 0;
	addr_t addr = 0;
	unsigned int pos = 0;
	char alias[3][SIZE_CMD_ARG_LINE + 1];

	for (i = 0; i < 3; ++i) {
		if (cmd_getnext(s, &pos, " \t", NULL, alias[i], sizeof(alias[i])) == NULL || *alias[i] == 0)
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

	if (phfs_regFile(alias[0], addr, sz) < 0)
		return ERR_ARG;

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_aliasReg(void)
{
	const static cmd_t alias_cmd = { .name = "alias", .run = cmd_alias, .info = cmd_aliasInfo };
	cmd_reg(&alias_cmd);
}
