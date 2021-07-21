/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Register phfs device
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
	lib_printf("registers device in phfs, usage: phfs [<alias> <major.minor> [protocol]]");
}


static int cmd_phfs(int argc, char *argv[])
{
	int res;
	char *endptr;
	unsigned int major, minor;

	if (argc == 1) {
		phfs_devsShow();
		return EOK;
	}
	else if (argc < 3 || argc > 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	/* Get major/minor */
	major = lib_strtoul(argv[2], &endptr, 0);
	if (*endptr != '.') {
		log_error("\n%s: Wrong major value for %s", argv[0], argv[1]);
		return -EINVAL;
	}

	minor = lib_strtoul(++endptr, &endptr, 0);
	if (*endptr != '\0') {
		log_error("\n%s: Wrong minor value for %s", argv[0], argv[1]);
		return -EINVAL;
	}

	if ((res = phfs_devReg(argv[1], major, minor, (argc == 3) ? NULL : argv[3])) < 0) {
		log_error("\nCan't register %s in phfs", argv[0]);
		return res;
	}

	log_info("\nRegistering phfs %s", argv[1]);

	return EOK;
}


__attribute__((constructor)) static void cmd_phfsReg(void)
{
	const static cmd_t app_cmd = { .name = "phfs", .run = cmd_phfs, .info = cmd_phfsInfo };

	cmd_reg(&app_cmd);
}
