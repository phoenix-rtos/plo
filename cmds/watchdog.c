/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Watchdog control
 *
 * Copyright 2024 Phoenix Systems
 * Author: Daniel Sawka
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>


static void cmd_watchdogInfo(void)
{
	lib_printf("shows watchdog state and controls autoreload behavior, usage: watchdog [auto|noauto|reload]");
}


static int cmd_watchdog(int argc, char *argv[])
{
	int res;
	imxrt_wdgInfo_t info;

	if (argc == 1) {
		res = _imxrt_wdgInfo(&info);
		if (res < 0) {
			log_error("\n%s: Could not read watchdog info", argv[0]);
			return CMD_EXIT_FAILURE;
		}

		lib_printf("\nWatchdog: %sabled (timeout: %u.%03us). Enabled at boot (by fuse): %s (timeout: %u.%03us)",
				(info.enabled != 0u) ? "en" : "dis",
				info.timeoutMs / 1000u, info.timeoutMs % 1000u,
				(info.platformEnabled != 0u) ? "yes" : "no",
				info.defaultTimeoutMs / 1000u, info.defaultTimeoutMs % 1000u);

		return CMD_EXIT_SUCCESS;
	}

	if (argc != 2) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (hal_strcmp(argv[1], "auto") == 0) {
		_imxrt_wdgAutoReload(1);
	}
	else if (hal_strcmp(argv[1], "noauto") == 0) {
		_imxrt_wdgAutoReload(0);
	}
	else if (hal_strcmp(argv[1], "reload") == 0) {
		_imxrt_wdgReload();
	}
	else {
		log_error("\n%s: Wrong arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	return CMD_EXIT_SUCCESS;
}


static cmd_t wait_cmd __attribute__((section("commands"), used)) = {
	.name = "watchdog", .run = cmd_watchdog, .info = cmd_watchdogInfo
};
