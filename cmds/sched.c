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

#define MAX_SCHED_WINDOWS 32U


static void cmd_schedInfo(void)
{
	lib_printf("configures scheduler, usage: sched [window1Size;window2Size...]");
}


static size_t cmd_listParse(char *maps, char sep)
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


static int cmd_sched(int argc, char *argv[])
{
	int res, i, argvID = 0;
	syspage_sched_window_t *window;

	char *schedWindowsTimes;
	size_t schedWinSz;
	time_t curTime = 0;

	/* Parse command arguments */
	if (argc > 2) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_0: command name */

	/* ARG_1: partition name */
	argvID = 1;
	schedWindowsTimes = argv[argvID++];

	schedWinSz = cmd_listParse(schedWindowsTimes, ';');
	if (schedWinSz > MAX_SCHED_WINDOWS) {
		log_error("\n%s: Too many scheduler windows (%d / max %d)", argv[0], schedWinSz, MAX_SCHED_WINDOWS);
		return CMD_EXIT_FAILURE;
	}

	for (i = 0; i < schedWinSz; ++i) {
		res = lib_strtoul(schedWindowsTimes, &schedWindowsTimes, 10);

		if ((window = syspage_schedWindowAdd()) == NULL) {
			log_error("\nCannot allocate memory for scheduler configuration");
			return -ENOMEM;
		}


		window->idx = i + 1;
		window->start = curTime;
		window->stop = curTime + res;
		curTime += res;
		schedWindowsTimes += 1; /* '\0' */
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t sched_cmd __attribute__((section("commands"), used)) = {
	.name = "sched", .run = cmd_sched, .info = cmd_schedInfo
};
