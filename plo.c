/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader console
 *
 * Copyright 2012, 2017, 2020-2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Authors: Pawel Pisarczyk, Lukasz Kosinski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>
#include <cmds/cmd.h>
#include <devices/devs.h>
#include <lib/console.h>


int main(void)
{
	hal_init();

	lib_printf(CONSOLE_CLEAR CONSOLE_BOLD "Phoenix-RTOS loader v. " VERSION);
	lib_printf(CONSOLE_CURSOR_HIDE CONSOLE_MAGENTA "\nhal: %s", hal_cpuInfo());
	devs_init();
	cmd_run();

	lib_printf(CONSOLE_CURSOR_SHOW CONSOLE_NORMAL);
	cmd_prompt();

	devs_done();
	hal_done();

	return 0;
}
