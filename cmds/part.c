/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Create partition
 *
 * Copyright 2020-2021 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "elf.h"

#include <lib/lib.h>
#include <hal/hal.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_partInfo(void)
{
	lib_printf("creates partition, usage: part <name> <allocmap1;allocmap2...> <accessmap1;accessmap2...>");
}


static int cmd_mapsAdd2Part(u8 *mapIDs, size_t nb, const char *mapNames)
{
	u8 id;
	int res, i;

	for (i = 0; i < nb; ++i) {
		if ((res = syspage_mapNameResolve(mapNames, &id)) < 0) {
			log_error("\nCan't add map %s", mapNames);
			return res;
		}

		mapIDs[i] = id;
		mapNames += hal_strlen(mapNames) + 1; /* name + '\0' */
	}

	return EOK;
}


static size_t cmd_mapsParse(char *maps, char sep)
{
	size_t nb = 0;

	while (*maps != '\0') {
		if (*maps == sep) {
			*maps = '\0';
			++nb;
		}
		maps++;
	}

	return ++nb;
}


static int cmd_part(int argc, char *argv[])
{
	int res, argvID = 0;

	char *allocMaps, *accessMaps;
	size_t allocSz, accessSz;

	const char *name;
	syspage_part_t *part;

	/* Parse command arguments */
	if (argc != 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_0: command name */

	/* ARG_1: partition name */
	argvID = 1;
	name = argv[argvID++];

	/* ARG_2: maps for allocations */
	allocMaps = argv[argvID++];

	/* ARG_3: accessible maps */
	accessMaps = argv[argvID];

	allocSz = cmd_mapsParse(allocMaps, ';');
	accessSz = cmd_mapsParse(accessMaps, ';');

	if ((part = syspage_partAdd()) == NULL ||
			(part->allocMaps = syspage_alloc(allocSz * sizeof(u8))) == NULL ||
			(part->accessMaps = syspage_alloc(accessSz * sizeof(u8))) == NULL ||
			(part->name = syspage_alloc(hal_strlen(name) + 1)) == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}
	cmd_mapsAdd2Part(part->allocMaps, allocSz, allocMaps);
	cmd_mapsAdd2Part(part->accessMaps, accessSz, accessMaps);
	part->allocMapSz = allocSz;
	part->accessMapSz = accessSz;
	hal_strcpy(part->name, name);

	if ((res = hal_getPartData(part, allocMaps, allocSz, accessMaps, accessSz)) < 0) {
		return res;
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t part_cmd __attribute__((section("commands"), used)) = {
	.name = "part", .run = cmd_part, .info = cmd_partInfo
};
