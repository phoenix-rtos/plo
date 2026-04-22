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
	lib_printf("creates partition, usage: part <name> <accessmap1;accessmap2...> <schedwindow> <memlimit>");
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
	size_t availableMem;
	hal_syspage_part_t *hal;
	syspage_part_t *part;
	syspage_sched_t *config;

	/* Parse command arguments */
	if (argc != 5) {
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

	part = syspage_partAdd();
	if (part == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}

	part->name = name;
	part->hal = hal;
	part->schedWindow = schedWindow;
	part->availableMem = availableMem;

	return CMD_EXIT_SUCCESS;
}


static const cmd_t part_cmd __attribute__((section("commands"), used)) = {
	.name = "part", .run = cmd_part, .info = cmd_partInfo
};
