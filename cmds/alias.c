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
	lib_printf("sets alias to file, usage: alias [<name> <offset> <size>]");
}


static int cmd_alias(int argc, char *argv[])
{
	char *end;
	size_t sz = 0;
	addr_t addr = 0;
	unsigned int i;

	if (argc == 1) {
		phfs_showFiles();
		return EOK;
	}
	else if (argc != 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}


	for (i = 2; i < 4; ++i) {
		if (i == 2)
			addr = lib_strtoul(argv[i], &end, 0);
		else if (i == 3)
			sz = lib_strtoul(argv[i], &end, 0);

		if (*end) {
			log_error("\n%s: Wrong arguments", argv[0]);
			return -EINVAL;
		}
	}

	if (phfs_regFile(argv[1], addr, sz) < 0) {
		log_error("\nCan't register file %s", argv[1]);
		return -EINVAL;
	}

	log_info("\nRegistering file %s ", argv[1]);

	return EOK;
}


__attribute__((constructor)) static void cmd_aliasReg(void)
{
	const static cmd_t alias_cmd = { .name = "alias", .run = cmd_alias, .info = cmd_aliasInfo };
	cmd_reg(&alias_cmd);
}
