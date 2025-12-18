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


#define ACC_MAPS_MAX 32


static void cmd_partInfo(void)
{
	lib_printf("creates partition, usage: part <name> <accessmap1;accessmap2...> <schedwindowNr1;schedwindowNr2...> <memlimit>");
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

	char *accessMaps, *schedWindows, *availableMem;
	size_t accessSz, schedWinSz;
	u32 schedWindowsMask;

	const char *name;
	syspage_part_t *part, *other;

	/* Parse command arguments */
	if (argc != 5) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_0: command name */

	/* ARG_1: partition name */
	argvID = 1;
	name = argv[argvID++];

	/* ARG_2: accessible maps */
	accessMaps = argv[argvID++];

	/* ARG_3: scheduler windows */
	schedWindows = argv[argvID++];

	/* ARG_4: memory allocation limit */
	availableMem = argv[argvID];

	accessSz = cmd_listParse(accessMaps, ';');
	schedWinSz = cmd_listParse(schedWindows, ';');

	part = syspage_partAdd();
	if (part == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}

	part->name = syspage_alloc(hal_strlen(name) + 1);
	if (part->name == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
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

	part->availableMem = lib_strtoul(availableMem, &availableMem, 0);
	if (*availableMem != '\0') {
		log_error("\n%s: Invalid arguments", argv[0]);
		return -EINVAL;
	}
	if (part->availableMem == 0) {
		part->availableMem = (size_t)-1;
	}
	hal_strcpy(part->name, name);

	part->hal = syspage_alloc(sizeof(*part->hal));
	if (part->hal == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}
	res = hal_getPartData(part->hal, NULL, 0, accessMaps, accessSz);
	if (res < 0) {
		return res;
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t part_cmd __attribute__((section("commands"), used)) = {
	.name = "part", .run = cmd_part, .info = cmd_partInfo
};
