/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Reboot system
 *
 * Copyright 2021 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "hal/armv7r/tda4vm/sciclient/sciclient.h"

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>


static void cmd_rebootInfo(void)
{
	lib_printf("reboot system (final state may depend on the latched boot config state)");
}


static int cmd_reboot(int argc, char *argv[])
{
	if (argc != 1) {
		log_error("\n%s: Command does not accept arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	log_info("\nRebooting\n");
	devs_done();
	hal_done();
	hal_interruptsDisableAll();
	hal_cpuReboot();

	/* Never reached */
	return CMD_EXIT_FAILURE;
}


static const cmd_t reboot_cmd __attribute__((section("commands"), used)) = {
	.name = "reboot", .run = cmd_reboot, .info = cmd_rebootInfo
};
