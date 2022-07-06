/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Start kernel
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <devices/devs.h>
#include <syspage.h>
#include <hal/hal.h>
#include <lib/lib.h>


static void cmd_goInfo(void)
{
	lib_printf("starts Phoenix-RTOS loaded into memory");
}


static int cmd_go(int argc, char *argv[])
{
	if (argc != 1) {
		log_error("\n%s: Command does not accept arguments", argv[0]);
		return -EINVAL;
	}

	log_info("\nRunning Phoenix-RTOS\n");
	lib_printf(CONSOLE_NORMAL CONSOLE_CURSOR_SHOW);

	devs_done();
	hal_done();
	hal_cpuJump();

	return EOK;
}


__attribute__((constructor)) static void cmd_goReg(void)
{
	const static cmd_t app_cmd = { .name = "go!", .run = cmd_go, .info = cmd_goInfo };

	cmd_reg(&app_cmd);
}
