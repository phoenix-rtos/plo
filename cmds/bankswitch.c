/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Switch Flash Banks
 *
 * Copyright 2022 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>


static void cmd_bankswitchInfo(void)
{
	lib_printf("switches flash banks, usage: bankswitch [0 | 1]");
}


static int cmd_bankswitch(int argc, char *argv[])
{
	int targetBank, err = CMD_EXIT_SUCCESS;

	if (argc == 1) {
		targetBank = (_stm32_getFlashBank() == 0) ? 1 : 0;
	}
	else if (argc == 2) {
		targetBank = lib_strtol(argv[1], NULL, 0);
	}
	else {
		log_error("\n%s: Wrong argument count", argv[0]);
		err = CMD_EXIT_FAILURE;
	}

	if (err == CMD_EXIT_SUCCESS) {
		_stm32_switchFlashBank(targetBank);
		log_info("\n%s: Bank switch successful (%d -> %d)", argv[0],
			(targetBank == 0) ? 1 : 0, targetBank);
	}

	return err;
}


static const cmd_t bankswitch_cmd __attribute__((section("commands"), used)) = {
	.name = "bankswitch", .run = cmd_bankswitch, .info = cmd_bankswitchInfo
};
