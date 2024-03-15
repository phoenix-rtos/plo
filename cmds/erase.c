/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Erase sectors or all data from flash device
 *
 * Copyright 2022-2024 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>

#include <phfs/phfs.h>


static void cmd_eraseInfo(void)
{
	lib_printf("erase sectors or all data from storage device using phfs interface, Usage: erase <dev> [<offset> <size>]");
}


static int cmd_erase(int argc, char *argv[])
{
	ssize_t res;
	size_t len = (size_t)-1;
	addr_t offs = 0;
	char *endptr = NULL;
	handler_t h;

	if (argc != 2 && argc != 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (argc != 2) {
		offs = lib_strtoul(argv[2], &endptr, 0);
		if (argv[2] == endptr) {
			log_error("\n%s: Wrong arguments", argv[0]);
			return CMD_EXIT_FAILURE;
		}
		len = lib_strtoul(argv[3], &endptr, 0);
		if (argv[3] == endptr) {
			log_error("\n%s: Wrong arguments", argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}

	res = phfs_open(argv[1], NULL, PHFS_OPEN_RAWONLY, &h);
	if (res < 0) {
		lib_printf("\n%s: Invalid phfs name provided: %s\n", argv[0], argv[1]);
		return CMD_EXIT_FAILURE;
	}

	res = lib_promptConfirm("\nWARNING!\nSerious risk of data loss, type %s to proceed.\n", "YES!", 10 * 1000);
	if (res != 1) {
		lib_printf("Aborted.\n");
		return CMD_EXIT_SUCCESS;
	}

	res = phfs_erase(h, offs, len, 0);
	if (res < 0) {
		lib_printf("\n%s: Error %zd", argv[0], res);
		(void)phfs_close(h);
		return CMD_EXIT_FAILURE;
	}

	(void)phfs_close(h);

	lib_printf("\nErased %zd bytes from %s phfs device.\n", res, argv[1]);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t erase_cmd __attribute__((section("commands"), used)) = {
	.name = "erase", .run = cmd_erase, .info = cmd_eraseInfo
};
