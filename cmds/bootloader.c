/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * enters ROM bootloader mode
 *
 * Copyright 2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <devices/devs.h>
#include <hal/hal.h>
#include <hal/armv7m/imxrt/bootloader.h>
#include <lib/lib.h>


static void cmd_bootloaderInfo(void)
{
	lib_printf("enters cpu vendor bootloader mode");
}


static int cmd_bootloader(int argc, char *argv[])
{
	if (argc != 1) {
		log_error("\n%s: Command does not accept arguments", argv[0]);
		return -EINVAL;
	}

	lib_printf("\nEntering ROM-API bootloader: %s\n", bootloader_getVendorString());

	devs_done();
	hal_done();
	hal_interruptsDisableAll();

	bootloader_run(BOOT_DOWNLOADER_USB);

	return EOK;
}


__attribute__((constructor)) static void cmd_bootloaderReg(void)
{
	const static cmd_t app_cmd = { .name = "bootloader", .run = cmd_bootloader, .info = cmd_bootloaderInfo };

	cmd_reg(&app_cmd);
}
