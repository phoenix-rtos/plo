/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Load kernel binary image
 *
 * Copyright 2021 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "elf.h"

#include <hal/hal.h>
#include <lib/lib.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_kernelimgInfo(void)
{
	lib_printf("loads Phoenix-RTOS binary image, usage: kernelimg <dev> [name] <text begin> <text size> <data begin> <data size>");
}


static int cmd_kernelimg(int argc, char *argv[])
{
	ssize_t res;
	const char *kname;
	handler_t handler;
	addr_t tbeg, dbeg, entryoff, kentry;
	size_t tsz, dsz;
	int indirect;

	const mapent_t *entry;

	/* Parse arguments */
	if (argc < 6 || argc > 7) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	kname = (argc == 7) ? argv[2] : PATH_KERNEL;

	if ((res = phfs_open(argv[1], kname, 0, &handler)) < 0) {
		log_error("\nCannot open %s, on %s", kname, argv[1]);
		return res;
	}

	hal_kernelGetEntryPointOffset(&entryoff, &indirect);
	if (indirect) {
		if ((res = phfs_read(handler, entryoff, &kentry, sizeof(kentry))) < 0) {
			log_error("\nCan't read %s, on %s", kname, argv[1]);
			phfs_close(handler);
			return res;
		}

	}
	else {
		kentry = entryoff;
	}

	tbeg = lib_strtoul(argv[argc - 4], NULL, 16);
	tsz = lib_strtoul(argv[argc - 3], NULL, 16);
	dbeg = lib_strtoul(argv[argc - 2], NULL, 16);
	dsz = lib_strtoul(argv[argc - 1], NULL, 16);

	if ((entry = syspage_entryAdd(NULL, tbeg, tsz, sizeof(long long))) == NULL) {
		log_error("\nCannot allocate memory for '%s'", kname);
		phfs_close(handler);
		return -ENOMEM;
	}

	if ((entry = syspage_entryAdd(NULL, dbeg, dsz, sizeof(long long))) == NULL) {
		log_error("\nCannot allocate memory for '%s'", kname);
		phfs_close(handler);
		return -ENOMEM;
	}

	hal_kernelEntryPoint(hal_kernelGetAddress(kentry));
	phfs_close(handler);

	log_info("\nLoaded %s", kname);

	return EOK;
}


__attribute__((constructor)) static void cmd_kernelimgReg(void)
{
	const static cmd_t app_cmd = { .name = "kernelimg", .run = cmd_kernelimg, .info = cmd_kernelimgInfo };

	cmd_reg(&app_cmd);
}
