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

#include "hal.h"
#include "lib.h"
#include "cmd.h"
#include "devs.h"
#include "console.h"


void plo_init(void)
{
	hal_init();

	lib_printf(CONSOLE_CLEAR);
	lib_printf(CONSOLE_BOLD);
	lib_printf("Phoenix-RTOS loader v. 1.21");

	lib_printf(CONSOLE_MAGENTA);
	lib_printf("\nhal: %s", hal_cpuInfo());
	devs_init();
	cmd_run();

	lib_printf(CONSOLE_NORMAL);
	cmd_prompt();

	devs_done();
	hal_done();
}
