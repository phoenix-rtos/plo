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
#include "hal.h"
#include "syspage.h"


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

	unsigned int argID = 0, argsc = 0;
	char (*args)[SIZE_CMD_ARG_LINE];

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	if (argsc == 0) {
		syspage_showMaps();
		return ERR_NONE;
	}
	else if (argsc < 4) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	namesz = (hal_strlen(args[argID]) < SIZE_MAP_NAME) ? (hal_strlen(args[argID]) + 1) : SIZE_MAP_NAME;
	hal_memcpy(mapname, args[argID], namesz);
	mapname[namesz] = '\0';

	++argID;
	start = lib_strtoul(args[argID], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	++argID;
	end = lib_strtoul(args[argID], &endptr, 0);
	if (hal_strlen(endptr) != 0) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	if (syspage_addmap(mapname, (void *)start, (void *)end, args[++argID]) < 0) {
		log_error("\nCan't create map %s", mapname);
		return ERR_ARG;
	}

	log_info("\nCreating map %s", mapname);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_mapReg(void)
{
	const static cmd_t app_cmd = { .name = "map", .run = cmd_map, .info = cmd_mapInfo };

	cmd_reg(&app_cmd);
}
