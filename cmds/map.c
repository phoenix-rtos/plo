/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Create memory map
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
#include <syspage.h>


static void cmd_mapInfo(void)
{
	lib_printf("defines multimap, usage: map [<name> <start> <end> <attributes>]");
}


static int cmd_map(int argc, char *argv[])
{
	int res;
	char *endptr;
	addr_t start, end;

	if (argc == 1) {
		syspage_mapShow();
		return EOK;
	}
	else if (argc != 5) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	start = lib_strtoul(argv[2], &endptr, 0);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return -EINVAL;
	}

	end = lib_strtoul(argv[3], &endptr, 0);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return -EINVAL;
	}


	if ((res = syspage_mapAdd(argv[1], start, end, argv[4])) < 0) {
		log_error("\nCan't create map %s", argv[1]);
		return res;
	}

	log_info("\nCreated map %s", argv[1]);

	return EOK;
}


__attribute__((constructor)) static void cmd_mapReg(void)
{
	const static cmd_t app_cmd = { .name = "map", .run = cmd_map, .info = cmd_mapInfo };

	cmd_reg(&app_cmd);
}
