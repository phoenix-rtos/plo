/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * phfs command
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


static void cmd_phfsInfo(void)
{
	lib_printf("registers device in phfs, usage: phfs <alias> <major.minor> [protocol]");
}


static int cmd_phfs(char *s)
{
	u16 argsc;
	char *endptr;
	unsigned int major, minor, pos = 0;
	char args[5][SIZE_CMD_ARG_LINE + 1];

	for (argsc = 0; argsc < 5; ++argsc) {
		if (cmd_getnext(s, &pos, ". \t", NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	if (argsc == 0) {
		phfs_showDevs();
		return ERR_NONE;
	}

	if (argsc < 3 || argsc > 4) {
		log_error("\ncmd/phfs: Wrong arguments");
		return ERR_ARG;
	}

	/* Get major/minor */
	major = lib_strtoul(args[1], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\ncmd/phfs: Wrong major value: %s", args[1]);
		return ERR_ARG;
	}

	minor = lib_strtoul(args[2], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\ncmd/phfs: Wrong minor value: %s", args[2]);
		return ERR_ARG;
	}

	if (phfs_regDev(args[0], major, minor, (argsc == 3) ? NULL : args[3]) < 0) {
		log_error("\ncmd/phfs: %s is not registered", args[0]);
		return ERR_ARG;
	}

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_phfsReg(void)
{
	const static cmd_t app_cmd = { .name = "phfs", .run = cmd_phfs, .info = cmd_phfsInfo };

	cmd_reg(&app_cmd);
}