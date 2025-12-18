/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Create partition
 *
 * Copyright 2026 Phoenix Systems
 * Author: Jakub Klimek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cmd.h"
#include "elf.h"

#include <lib/lib.h>
#include <hal/hal.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_partInfo(void)
{
	lib_printf("creates partition, usage: part <name> [flags] <allocmap1;allocmap2...> <accessmap1;accessmap2...>");
}


static int cmd_mapsAddToPart(u8 *mapIDs, size_t nb, const char *mapNames)
{
	u8 id;
	size_t i;
	int res;

	for (i = 0; i < nb; ++i) {
		res = syspage_mapNameResolve(mapNames, &id);
		if (res < 0) {
			log_error("\nCan't add map %s (%d)", mapNames, res);
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

	unsigned int i, flags = 0;

	const char *name;
	syspage_part_t *part;

	/* Parse command arguments */
	if ((argc < 4) || (argc > 5)) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_0: command name */

	/* ARG_1: partition name */
	argvID = 1;
	name = argv[argvID++];

	/* ARG_2: optional flags */
	if (argv[argvID][0] == '-') {
		for (i = 1; argv[argvID][i] != '\0'; ++i) {
			if ((argv[argvID][i] | 0x20) == 'm') {
				flags |= pFlagIPCAll;
			}
			else if ((argv[argvID][i] | 0x20) == 's') {
				flags |= pFlagSpawnAll;
			}
			else {
				log_error("\n%s: Wrong arguments", argv[0]);
				return CMD_EXIT_FAILURE;
			}
		}
		argvID++;
	}

	/* ARG_3: maps for allocations */
	allocMaps = argv[argvID++];

	/* ARG_4: accessible maps */
	accessMaps = argv[argvID];

	allocSz = cmd_mapsParse(allocMaps, ';');
	accessSz = cmd_mapsParse(accessMaps, ';');

	part = syspage_partAdd();
	if (part == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}

	part->allocMaps = syspage_alloc(allocSz * sizeof(u8));
	part->accessMaps = syspage_alloc(accessSz * sizeof(u8));
	part->name = syspage_alloc(hal_strlen(name) + 1);
	if ((part->allocMaps == NULL) ||
			(part->accessMaps == NULL) ||
			(part->name == NULL)) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}

	res = cmd_mapsAddToPart(part->allocMaps, allocSz, allocMaps);
	if (res < 0) {
		return res;
	}
	res = cmd_mapsAddToPart(part->accessMaps, accessSz, accessMaps);
	if (res < 0) {
		return res;
	}
	part->allocMapSz = allocSz;
	part->accessMapSz = accessSz;
	part->flags = flags;
	hal_strcpy(part->name, name);

	res = hal_getPartData(part, allocMaps, allocSz, accessMaps, accessSz);
	if (res < 0) {
		return res;
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t part_cmd __attribute__((section("commands"), used)) = {
	.name = "part", .run = cmd_part, .info = cmd_partInfo
};
