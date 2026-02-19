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
	lib_printf("creates partition, usage: part <name> <allocmap1;allocmap2...> <accessmap1;accessmap2...> <schedwindowNr1;schedwindowNr2...> <memlimit>");
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


static int cmd_schedWindowAdd2Part(u32 *schedIds, size_t nb, char *schedStr)
{
	int res, i;

	for (i = 0; i < nb; ++i) {
		res = lib_strtoul(schedStr, &schedStr, 10);


		if (res >= syspage_schedulerWindowCount()) {
			log_error("\nInvalid scheduler window index");
			return -EINVAL;
		}

		*schedIds |= 1U << res;
		schedStr += 1; /* '\0' */
	}
	return EOK;
}


static size_t cmd_listParse(char *list, char sep)
{
	size_t nb = 0;

	while (*list != '\0') {
		if (*list == sep) {
			*list = '\0';
			++nb;
		}
		list++;
	}

	return ++nb;
}


static int cmd_part(int argc, char *argv[])
{
	int res, argvID = 0;

	char *allocMaps, *accessMaps, *schedWindows, *availableMem;
	size_t allocSz, accessSz, schedWinSz;

	const char *name;
	syspage_part_t *part, *other;

	/* Parse command arguments */
	if (argc != 6) {
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
	accessMaps = argv[argvID++];

	/* ARG_4: scheduler windows */
	schedWindows = argv[argvID++];

	/* ARG_5: memory allocation limit */
	availableMem = argv[argvID];

	allocSz = cmd_listParse(allocMaps, ';');
	accessSz = cmd_listParse(accessMaps, ';');
	schedWinSz = cmd_listParse(schedWindows, ';');

	if ((part = syspage_partAdd()) == NULL ||
			(part->allocMaps = syspage_alloc(allocSz * sizeof(u8))) == NULL ||
			(part->accessMaps = syspage_alloc(accessSz * sizeof(u8))) == NULL ||
			(part->name = syspage_alloc(hal_strlen(name) + 1)) == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}
	cmd_mapsAdd2Part(part->allocMaps, allocSz, allocMaps);
	cmd_mapsAdd2Part(part->accessMaps, accessSz, accessMaps);
	if ((res = cmd_schedWindowAdd2Part(&part->schedWindowsMask, schedWinSz, schedWindows)) < 0) {
		return res;
	}

	other = syspage_partsGet();
	do {
		if (((part->schedWindowsMask & other->schedWindowsMask) != 0U) &&
				(part->schedWindowsMask != other->schedWindowsMask)) {
			log_error("\nUnsupported scheduler windows configuration for partitions %s and %s", name, other->name);
			return -EINVAL;
		}
		other = other->next;
	} while (other != syspage_partsGet());

	part->allocMapSz = allocSz;
	part->accessMapSz = accessSz;
	part->availableMem = lib_strtoul(availableMem, &availableMem, 16);
	if (part->availableMem == 0) {
		part->availableMem = (unsigned long)-1;
	}
	hal_strcpy(part->name, name);

	if ((res = hal_getPartData(part, allocMaps, allocSz, accessMaps, accessSz)) < 0) {
		return res;
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t part_cmd __attribute__((section("commands"), used)) = {
	.name = "part", .run = cmd_part, .info = cmd_partInfo
};
