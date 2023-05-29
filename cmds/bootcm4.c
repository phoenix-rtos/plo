/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * boot CM4 CPU i.MX RT117x
 *
 * Copyright 2022-2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <phfs/phfs.h>
#include <lib/lib.h>


#define CM4_BOOT_MEMSIZE 0x00040000u
#define CM4_BOOT_ADDRESS 0x20200000u


static struct {
	unsigned int isImgLoaded;
	addr_t cm4VtorOffset;
} common = { 0 };


static void cmd_bootcm4Info(void)
{
	lib_printf("boots CM4 core image, use <-h> to see usage");
}


static void print_usage(const char *name)
{
	lib_printf(
		"Usage: %s <options> [<device> <image>]\n"
		"\t-b  boot CM4 core\n"
		"\t-l  load binary image using phfs <device> and <image>\n"
		"\t-o  optional offset to vectors table (default=0)\n",
		name);
}


static ssize_t loadImagePhfs(const char *phfsDev, const char *phfsFile)
{
	u8 buff[SIZE_MSG_BUFF];
	handler_t h;
	addr_t offs = 0u;
	addr_t addr = CM4_BOOT_ADDRESS;

	ssize_t res = phfs_open(phfsDev, phfsFile, 0, &h);

	if (res < 0) {
		log_error("Can't open '%s' on '%s'", phfsFile, phfsDev);
		return res;
	}

	do {
		res = phfs_read(h, offs, buff, sizeof(buff));
		if (res < 0) {
			log_error("Can't read '%s' from '%s'", phfsFile, phfsDev);
			break;
		}

		offs += res;
		if (offs > CM4_BOOT_MEMSIZE) {
			log_error("Image '%s' too big", phfsFile);
			res = -ENOSPC;
			break;
		}

		hal_memcpy((void *)addr, buff, res);
		addr += res;
	} while (res > 0);

	phfs_close(h);

	return res;
}


static int cmd_bootcm4(int argc, char *argv[])
{
	char *endptr;
	const char *phfsDev = NULL;
	const char *phfsFile = NULL;

	int opt;
	int optLoad = 0;
	int optBoot = 0;
	addr_t tmp;

	lib_printf("\n");

	if (argc == 1) {
		print_usage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	for (;;) {
		opt = lib_getopt(argc, argv, "bhlo:");
		if (opt == -1) {
			break;
		}

		switch (opt) {
			case 'b':
				optBoot = 1;
				break;

			case 'l':
				optLoad = 1;
				break;

			case 'o':
				tmp = (addr_t)lib_strtoul(optarg, &endptr, 0);
				if ((*endptr != '\0') || (tmp >= CM4_BOOT_MEMSIZE)) {
					log_error("%s: Invalid arguments", argv[0]);
					return CMD_EXIT_FAILURE;
				}
				common.cm4VtorOffset = tmp;

				break;

			case 'h':
			default:
				print_usage(argv[0]);
				return CMD_EXIT_FAILURE;
		}
	}

	if (argc > optind) {
		if (((argc - optind) != 2) || (optLoad == 0)) {
			log_error("%s: Invalid arguments", argv[0]);
			return CMD_EXIT_FAILURE;
		}
		phfsDev = argv[optind++];
		phfsFile = argv[optind++];
	}

	if (_imxrt_getStateCM4() != 0u) {
		if ((optLoad != 0) || (optBoot != 0)) {
			log_error("%s: CM4 core is already running", argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}

	if (optLoad != 0) {
		if ((phfsDev == NULL) || (phfsFile == NULL)) {
			log_error("%s: Missing argument <device file> to load", argv[0]);
			return CMD_EXIT_FAILURE;
		}

		if (loadImagePhfs(phfsDev, phfsFile) < 0) {
			return CMD_EXIT_FAILURE;
		}

		hal_cleaninvalDCacheAddr((void *)CM4_BOOT_ADDRESS, CM4_BOOT_MEMSIZE);

		common.isImgLoaded++;
	}

	if (optBoot != 0) {
		if (common.isImgLoaded == 0u) {
			log_error("%s: CM4 core image not loaded", argv[0]);
			return CMD_EXIT_FAILURE;
		}

		if (_imxrt_setVtorCM4(0, 0, (CM4_BOOT_ADDRESS + common.cm4VtorOffset)) < 0) {
			log_error("%s: Unable to init CM4 core vectors", argv[0]);
			return CMD_EXIT_FAILURE;
		}

		_imxrt_runCM4();
	}

	return CMD_EXIT_SUCCESS;
}


__attribute__((constructor)) static void cmd_bootcm4Reg(void)
{
	const static cmd_t app_cmd = {
		.name = "bootcm4", .run = cmd_bootcm4, .info = cmd_bootcm4Info
	};

	cmd_reg(&app_cmd);
}
