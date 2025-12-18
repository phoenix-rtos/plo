/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Configure scheduler
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
	unsigned int i;
	unsigned long duration;
	syspage_sched_window_t *window;

	char *schedWindowsTimes;
	size_t schedWinSz;
	time_t curTime = 0;

	/* Parse command arguments */
	if (argc > 2) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (syspage_schedulerWindowCount() > 1) {
		log_error("\n%s: Scheduler windows are already configured!", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_0: command name */

	/* ARG_1: duration list */
	if (argc == 2) {
		schedWindowsTimes = argv[1];
		schedWinSz = cmd_listParse(schedWindowsTimes, ';');
	}
	else {
		schedWindowsTimes = "";
		schedWinSz = 0;
	}

	if (schedWinSz > MAX_SCHED_WINDOWS) {
		log_error("\n%s: Too many scheduler windows (%zu / max %u)", argv[0], schedWinSz, MAX_SCHED_WINDOWS);
		return CMD_EXIT_FAILURE;
	}

	for (i = 0; i < schedWinSz; ++i) {
		duration = lib_strtoul(schedWindowsTimes, &schedWindowsTimes, 0);
		if (*schedWindowsTimes != '\0') {
			log_error("\n%s: Window duration is not a valid number", argv[0]);
			return CMD_EXIT_FAILURE;
		}
		window = syspage_schedWindowAdd();
		if (window == NULL) {
			log_error("\n%s: Cannot allocate memory for scheduler configuration", argv[0]);
			return CMD_EXIT_FAILURE;
		}

		window->id = i + 1U;
		window->stop = curTime + duration;
		curTime += duration;
		schedWindowsTimes += 1; /* '\0' */
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t sched_cmd __attribute__((section("commands"), used)) = {
	.name = "sched", .run = cmd_sched, .info = cmd_schedInfo
};
