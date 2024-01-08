/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Create file alias
 *
 * Copyright 2021, 2023 Phoenix Systems
 * Author: Hubert Buczynski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <phfs/phfs.h>
#include <lib/getopt.h>
#include <lib/log.h>


static addr_t aliasBase = 0;


static void cmd_aliasInfo(void)
{
	lib_printf("sets alias to file, usage: alias [-b <base> | [-r] <name> <offset> <size>]");
}


static int cmd_alias(int argc, char *argv[])
{
	char *end;
	size_t sz = 0;
	addr_t addr = 0;
	int c, relative = 0;
	addr_t newBase;

	if (argc == 1) {
		if (aliasBase != 0) {
			lib_printf("\nRelative alias base: 0x%p", aliasBase);
		}
		phfs_aliasesShow();
		return CMD_EXIT_SUCCESS;
	}

	for (;;) {
		c = lib_getopt(argc, argv, "rb:");
		if (c < 0) {
			break;
		}

		switch (c) {
			case 'r':
				relative = 1;
				break;

			case 'b':
				if (argc != 3) {
					log_error("\n%s: Invalid argument count.\n", argv[0]);
					cmd_aliasInfo();
					return CMD_EXIT_FAILURE;
				}

				newBase = lib_strtoul(optarg, &end, 0);
				if (*end != '\0') {
					log_error("\n%s: Invalid base.\n", argv[0]);
					cmd_aliasInfo();
					return CMD_EXIT_FAILURE;
				}

				aliasBase = newBase;
				lib_printf("\n%s: Setting relative base address to 0x%p", argv[0], newBase);

				/* Ignore everything else, this is a separate function */
				return CMD_EXIT_SUCCESS;

			default:
				log_error("\n%s: Invalid option: %c\n", argv[0], c);
				cmd_aliasInfo();
				return CMD_EXIT_FAILURE;
		}
	}

	if ((argc - optind) != 3) {
		log_error("\n%s: Wrong argument count %d", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	addr = lib_strtoul(argv[optind + 1], &end, 0);
	if (*end != '\0') {
		log_error("\n%s: Wrong arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	sz = lib_strtoul(argv[optind + 2], &end, 0);
	if (*end != '\0') {
		log_error("\n%s: Wrong arguments", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (relative != 0) {
		if (addr + aliasBase < addr) {
			log_error("\n%s: Relative offset overflow", argv[0]);
			return CMD_EXIT_FAILURE;
		}
		addr += aliasBase;
	}

	if (phfs_aliasReg(argv[optind], addr, sz) < 0) {
		log_error("\n%s: Can't register file %s", argv[0], argv[optind]);
		return CMD_EXIT_FAILURE;
	}

	log_info("\n%s: Registering file %s ", argv[0], argv[optind]);

	return CMD_EXIT_SUCCESS;
}


__attribute__((constructor)) static void cmd_aliasReg(void)
{
	const static cmd_t alias_cmd = { .name = "alias", .run = cmd_alias, .info = cmd_aliasInfo };
	cmd_reg(&alias_cmd);
}
