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
	lib_printf("creates partition, usage: part <name> <accessmap1;accessmap2...> <schedwindow> <memlimit> [-itcp]");
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
	size_t accessSz;

	char *name;
	unsigned char schedWindow;
	size_t availableMem, i;
	char *accessMap;
	u8 id;
	hal_syspage_part_t *hal;
	syspage_part_t *part;
	syspage_sched_t *config;
	unsigned int flags = 0;

	/* Parse command arguments */
	if (argc < 5 || argc > 6) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_0: command name */

	argvID = 1;

	/* ARG_1: partition name */
	name = syspage_alloc(hal_strlen(argv[argvID]) + 1);
	if (name == NULL) {
		log_error("\nCannot allocate memory for %s", argv[argvID]);
		return -ENOMEM;
	}
	hal_strcpy(name, argv[argvID]);

	argvID++;

	/* ARG_2: accessible maps */
	accessSz = cmd_listParse(argv[argvID], ';');
	if (accessSz > sizeof(part->maps) / sizeof(part->maps[0])) {
		log_error("\n%s: Too many access maps in partition %s", argv[argvID], name);
		return -EINVAL;
	}
	accessMap = argv[argvID];
	for (i = 0; i < accessSz; ++i) {
		if (syspage_mapNameResolve(accessMap, &id) < 0) {
			log_error("\n%s: Invalid map name", accessMap);
			return -EINVAL;
		}
		accessMap += hal_strlen(accessMap) + 1U;
	}
	accessMap = argv[argvID];

	hal = syspage_alloc(sizeof(*hal));
	if (hal == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}
	res = hal_getPartData(hal, NULL, 0, argv[argvID], accessSz);
	if (res < 0) {
		return res;
	}

	argvID++;

	/* ARG_3: scheduler windows */
	schedWindow = lib_strtoul(argv[argvID], &argv[argvID], 10);
	if (*argv[argvID] != '\0') {
		log_error("\n%s: Invalid arguments", argv[0]);
		return -EINVAL;
	}
	config = syspage_schedulerConfigGet();
	if ((config == NULL) || (schedWindow > config->windowCnt)) {
		log_error("\n%s: Invalid scheduler window number", argv[0]);
		return -EINVAL;
	}

	argvID++;

	/* ARG_4: memory allocation limit */
	availableMem = lib_strtoul(argv[argvID], &argv[argvID], 0);
	if (*argv[argvID] != '\0') {
		log_error("\n%s: Invalid arguments", argv[0]);
		return -EINVAL;
	}
	if (availableMem == 0) {
		availableMem = (size_t)-1;
	}

	argvID++;

	/* Flags */
	if (argvID < argc) {
		if (argv[argvID][0] != '-') {
			log_error("\n%s: Invalid arguments", argv[0]);
			return -EINVAL;
		}
		i = 1;
		while (argv[argvID][i] != '\0') {
			switch (argv[argvID][i++]) {
				case 'i':
					flags |= pFlagIntr;
					break;
				case 't':
					flags |= pFlagTime;
					break;
				case 'c':
					flags |= pFlagPctl;
					break;
				case 'p':
					flags |= pFlagPerf;
					break;
				default:
					log_error("\n%s: Invalid arguments", argv[0]);
					return -EINVAL;
			}
		}
	}

	part = syspage_partAdd();
	if (part == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}

	part->name = name;
	part->hal = hal;
	part->schedWindow = schedWindow;
	part->availableMem = availableMem;
	part->flags = flags;
	for (i = 0; i < accessSz; ++i) {
		syspage_mapNameResolve(accessMap, &part->maps[i]);
		accessMap += hal_strlen(accessMap) + 1U;
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t part_cmd __attribute__((section("commands"), used)) = {
	.name = "part", .run = cmd_part, .info = cmd_partInfo
};
