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
		return CMD_EXIT_SUCCESS;
	}
	else if (argc != 5) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	start = lib_strtoul(argv[2], &endptr, 0);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	end = lib_strtoul(argv[3], &endptr, 0);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}


	res = syspage_mapAdd(argv[1], start, end, argv[4]);
	if (res < 0) {
		log_error("\nCan't create map %s (%d)", argv[1], res);
		return CMD_EXIT_FAILURE;
	}

	log_info("\nCreated map %s", argv[1]);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t map_cmd __attribute__((section("commands"), used)) = {
	.name = "map", .run = cmd_map, .info = cmd_mapInfo
};
