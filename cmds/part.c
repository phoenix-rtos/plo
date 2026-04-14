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
	lib_printf("creates partition, usage: part <name> [flags] <allocmap1;allocmap2...> <accessmap1;accessmap2...> <schedwindowNr1;schedwindowNr2...> <memlimit>");
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


static int cmd_schedWindowAddToPart(u32 *schedIds, size_t nb, char *schedStr, const char *cmd)
{
	unsigned long window;
	size_t i;

	*schedIds = 0;

	for (i = 0; i < nb; ++i) {
		window = lib_strtoul(schedStr, &schedStr, 10);
		if (*schedStr != '\0') {
			log_error("\n%s: Scheduler window is not a valid number", cmd);
			return -EINVAL;
		}
		if (window >= syspage_schedulerWindowCount()) {
			log_error("\n%s: Invalid scheduler window index", cmd);
			return -EINVAL;
		}

		*schedIds |= 1U << window;
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

	char *allocMaps, *accessMaps, *schedWindows, *availableMem, *availableKMem;
	size_t allocSz, accessSz, schedWinSz;
	u32 schedWindowsMask;

	unsigned int i, flags = 0;

	const char *name;
	syspage_part_t *part, *other;

	/* Parse command arguments */
	if ((argc < 7) || (argc > 8)) {
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
	accessMaps = argv[argvID++];

	/* ARG_5: scheduler windows */
	schedWindows = argv[argvID++];

	/* ARG_6: memory allocation limit */
	availableMem = argv[argvID++];

	/* ARG_7: kernel memory allocation limit */
	availableKMem = argv[argvID];

	allocSz = cmd_listParse(allocMaps, ';');
	accessSz = cmd_listParse(accessMaps, ';');
	schedWinSz = cmd_listParse(schedWindows, ';');

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
	res = cmd_schedWindowAddToPart(&schedWindowsMask, schedWinSz, schedWindows, argv[0]);
	if (res < 0) {
		return res;
	}
	part->schedWindowsMask = schedWindowsMask;

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
	part->availableMem = lib_strtoul(availableMem, &availableMem, 0);
	if (*availableMem != '\0') {
		log_error("\n%s: Invalid arguments", argv[0]);
		return -EINVAL;
	}
	part->availableKMem = lib_strtoul(availableKMem, &availableKMem, 0);
	if (*availableKMem != '\0') {
		log_error("\n%s: Invalid arguments", argv[0]);
		return -EINVAL;
	}
	if (part->availableMem == 0) {
		part->availableMem = (size_t)-1;
	}
	if (part->availableKMem == 0) {
		part->availableKMem = (size_t)-1;
	}
	part->usedMem = 0;
	part->usedKMem = 0;
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
