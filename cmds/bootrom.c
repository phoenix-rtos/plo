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
#include <hal/armv7m/imxrt/bootrom.h>
#include <lib/lib.h>


static void cmd_bootromInfo(void)
{
	lib_printf("enters cpu vendor bootloader mode use <-h> to see usage");
}


static void print_usage(const char *name)
{
	lib_printf(
		"Usage: %s <-b | -s imagenum>\n"
		"where:\n"
		"\t-b enters USB serial downloader mode\n"
		"\t-s <imagenum> selects image <1, 2, 3 or 4> to boot from\n",
		name);
}


static int cmd_bootrom(int argc, char *argv[])
{
	int ret;
	int opt;
	int optBootMode = 0;
	int optImageSel = 0;

	lib_printf("\n");

	ret = bootrom_init();
	if (ret < 0) {
		log_error("%s: ROM bootloader unavailable", argv[0]);
		return ret;
	}

	if (argc == 1) {
		print_usage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	for (;;) {
		opt = lib_getopt(argc, argv, "bhs:");
		if (opt == -1) {
			break;
		}

		switch (opt) {
			case 'b':
				optBootMode = BOOT_DOWNLOADER_USB;
				break;

			case 's':
				optImageSel = (int)lib_strtol(optarg, NULL, 0);
				if ((optImageSel < 1) || (optImageSel > 4)) {
					log_error("%s: invalid image selection <1, 4>\n", argv[0]);
					print_usage(argv[0]);
					return CMD_EXIT_FAILURE;
				}
				break;

			case 'h':
			default:
				print_usage(argv[0]);
				return CMD_EXIT_FAILURE;
		}
	}

	if ((optBootMode != 0) && (optImageSel != 0)) {
		log_error("%s: select either boot mode or image number, not both\n", argv[0]);
		print_usage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (optBootMode != 0) {
		lib_printf("USB Serial downloader mode\n");
	}
	else {
		lib_printf("Selected image to boot from: %d\n", optImageSel);
	}

	lib_printf("Entering ROM-API bootloader: %s\n", bootrom_getVendorString());

	devs_done();
	hal_done();
	hal_interruptsDisableAll();

	/*
	 * TODO: Enable 60sec watchdog on imxrt1064,
	 * like on imxrt1176 is enabled by default
	 */

	if (optImageSel > 0) {
		optImageSel--;
	}

	bootrom_run(optBootMode | BOOT_IMAGE_SELECT(optImageSel));

	/* Never reached */

	return CMD_EXIT_SUCCESS;
}


__attribute__((constructor)) static void cmd_bootromReg(void)
{
	const static cmd_t app_cmd = {
		.name = "bootrom", .run = cmd_bootrom, .info = cmd_bootromInfo
	};

	cmd_reg(&app_cmd);
}
