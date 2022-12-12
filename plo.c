/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Initial loader's routines
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
#include <syspage.h>

static void nop(void)
{
}

void hal_customInit(void) __attribute__((weak, alias("nop")));
void hal_customDone(void) __attribute__((weak, alias("nop")));

int main(void)
{
	hal_init();
	hal_customInit();
	syspage_init();

	lib_printf(CONSOLE_BOLD "Phoenix-RTOS loader v. " VERSION CONSOLE_NORMAL);
	lib_printf(CONSOLE_CURSOR_HIDE CONSOLE_MAGENTA "\nhal: %s", hal_cpuInfo());
	devs_init();
	cmd_run();

	lib_printf(CONSOLE_CURSOR_SHOW CONSOLE_NORMAL);
	cmd_prompt();

	devs_done();
	hal_done();
	hal_customDone();

	return 0;
}
