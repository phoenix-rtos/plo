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

#include <hal/hal.h>
#include <phfs/phfs.h>


static void cmd_phfsInfo(void)
{
	lib_printf("registers device in phfs, usage: phfs <alias> <major.minor> [protocol]");
}


static int cmd_phfs(char *s)
{
	char *endptr;
	cmdarg_t *args;
	unsigned int major, minor, argsc;

	argsc = cmd_getArgs(s, ". \t", &args);
	if (argsc == 0) {
		phfs_showDevs();
		return EOK;
	}
	else if (argsc < 3 || argsc > 4) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	/* Get major/minor */
	major = lib_strtoul(args[1], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong major value %s for %s", args[1], args[0]);
		return -EINVAL;
	}

	minor = lib_strtoul(args[2], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong minor value %s for %s", args[2], args[0]);
		return -EINVAL;
	}

	if (phfs_regDev(args[0], major, minor, (argsc == 3) ? NULL : args[3]) < 0) {
		log_error("\nCan't register %s in phfs", args[0]);
		return -EINVAL;
	}

	log_info("\nRegistering phfs %s", args[0]);

	return EOK;
}


__attribute__((constructor)) static void cmd_phfsReg(void)
{
	const static cmd_t app_cmd = { .name = "phfs", .run = cmd_phfs, .info = cmd_phfsInfo };

	cmd_reg(&app_cmd);
}
