/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * List device drivers
 *
 * Copyright 2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <devices/devs.h>

#include "cmd.h"

#define DEVS_HEADER_FORMAT "%5s %5s %-10s %s"
#define DEVS_ENTRY_FORMAT  "%5u %5u %-10s %s"


static void cmd_devsInfo(void)
{
	lib_printf("enumerates registered device drivers");
}


static const char *devClassName(unsigned int major)
{
	static const char *className[] = {
		"uart", "usb", "storage", "tty", "ram", "nand-data", "nand-meta", "nand-raw", "pipe"
	};

	return (major < (sizeof(className) / sizeof(className[0]))) ?
		className[major] :
		"unknown";
}


static int cmd_devs(int argc, char *argv[])
{
	unsigned int ctx = 0;
	unsigned int major = 0;
	unsigned int minor = 0;

	if (argc != 1) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	lib_printf("\n\033[1m" DEVS_HEADER_FORMAT "\033[0m\n", "MAJOR", "MINOR", "CLASS", "DRIVER NAME");

	for (;;) {
		const dev_t *dev = devs_iterNext(&ctx, &major, &minor);
		if (dev == DEVS_ITER_STOP) {
			break;
		}

		if (dev != NULL) {
			lib_printf(DEVS_ENTRY_FORMAT "\n", major, minor, devClassName(major), dev->name);
		}
	}

	return CMD_EXIT_SUCCESS;
}


static const cmd_t devices_cmd __attribute__((section("commands"), used)) = {
	.name = "devices", .run = cmd_devs, .info = cmd_devsInfo
};
