/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * map command
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
	lib_printf("defines multimap, usage: map <start> <end> <name> <attributes>");
}


static int cmd_map(char *s)
{
	char *endptr;
	addr_t start, end;

	u8 namesz;
	char mapname[SIZE_MAP_NAME + 1];

	unsigned int argsc = 0;
	cmdarg_t *args;

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	if (argsc == 0) {
		syspage_showMaps();
		return EOK;
	}
	else if (argsc != 4) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	namesz = hal_strlen(args[0]);
	if (namesz >= sizeof(mapname))
		namesz = sizeof(mapname) - 1;
	hal_memcpy(mapname, args[0], namesz);
	mapname[namesz] = '\0';

	start = lib_strtoul(args[1], &endptr, 0);
	if (*endptr) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	end = lib_strtoul(args[2], &endptr, 0);
	if (*endptr) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	if (syspage_addmap(mapname, (void *)start, (void *)end, args[3]) < 0) {
		log_error("\nCan't create map %s", mapname);
		return -EINVAL;
	}

	log_info("\nCreating map %s", mapname);

	return EOK;
}


__attribute__((constructor)) static void cmd_mapReg(void)
{
	const static cmd_t app_cmd = { .name = "map", .run = cmd_map, .info = cmd_mapInfo };

	cmd_reg(&app_cmd);
}
