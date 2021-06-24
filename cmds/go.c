/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * go command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <devices/devs.h>
#include <syspage.h>
#include <hal/hal.h>
#include <lib/log.h>
#include <lib/console.h>


static void cmd_goInfo(void)
{
	lib_printf("starts Phoenix-RTOS loaded into memory");
}


static int cmd_go(int argc, char *argv[])
{
	addr_t kernel_entry;
	int res;

	if (argc != 1) {
		log_error("\n%s: Command does not accept arguments", argv[0]);
		return -EINVAL;
	}

	if ((res = syspage_validateKernel(&kernel_entry)) != EOK) {
		log_error("\nValid kernel image has not been loaded.");
		return res;
	}

	log_info("\nRunning Phoenix-RTOS");
	lib_printf(CONSOLE_NORMAL CONSOLE_CLEAR CONSOLE_CURSOR_SHOW);
	devs_done();
	hal_cpuJump(kernel_entry);
	return EOK;
}


__attribute__((constructor)) static void cmd_goReg(void)
{
	const static cmd_t app_cmd = { .name = "go!", .run = cmd_go, .info = cmd_goInfo };

	cmd_reg(&app_cmd);
}
