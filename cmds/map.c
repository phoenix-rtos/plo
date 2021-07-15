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

	size_t namesz;
	char mapname[SIZE_MAP_NAME + 1];

	if (argc == 1) {
		syspage_showMaps();
		return EOK;
	}
	else if (argc != 5) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	namesz = hal_strlen(argv[1]);
	if (namesz >= sizeof(mapname))
		namesz = sizeof(mapname) - 1;
	hal_memcpy(mapname, argv[1], namesz);
	mapname[namesz] = '\0';

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

	if ((res = syspage_addmap(mapname, start, end, argv[4])) < 0) {
		log_error("\nCan't create map %s", mapname);
		return res;
	}

	log_info("\nCreating map %s", mapname);

	return EOK;
}


__attribute__((constructor)) static void cmd_mapReg(void)
{
	const static cmd_t app_cmd = { .name = "map", .run = cmd_map, .info = cmd_mapInfo };

	cmd_reg(&app_cmd);
}
