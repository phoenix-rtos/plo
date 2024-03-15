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
	lib_printf("loads Phoenix-RTOS binary image (only for XIP from read only memory), usage: kernelimg <dev> [name] <text begin> <text size> <data begin> <data size>");
}


static int cmd_kernelimg(int argc, char *argv[])
{
	ssize_t res;
	const char *kname;
	handler_t handle;
	addr_t tbeg, dbeg, entryoff, kentry, offs, addr;
	size_t tsz, dsz;
	int indirect;
	char *endptr;
	phfs_stat_t stat;
	const mapent_t *entry;

	/* Parse arguments */
	if ((argc < 6) || (argc > 7)) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	kname = (argc == 7) ? argv[2] : PATH_KERNEL;

	res = phfs_open(argv[1], kname, 0, &handle);
	if (res < 0) {
		log_error("\nCannot open %s, on %s (%d)", kname, argv[1], res);
		return CMD_EXIT_FAILURE;
	}

	hal_kernelGetEntryPointOffset(&entryoff, &indirect);
	if (indirect) {
		res = phfs_read(handle, entryoff, &kentry, sizeof(kentry));
		if (res < 0) {
			log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
			phfs_close(handle);
			return CMD_EXIT_FAILURE;
		}
	}
	else {
		kentry = entryoff;
	}

	tbeg = lib_strtoul(argv[argc - 4], &endptr, 16);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	tsz = lib_strtoul(argv[argc - 3], &endptr, 16);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	dbeg = lib_strtoul(argv[argc - 2], &endptr, 16);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	dsz = lib_strtoul(argv[argc - 1], &endptr, 16);
	if (*endptr) {
		log_error("\n%s: Wrong arguments", argv[0]);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	if (phfs_aliasAddrResolve(handle, &offs) < 0) {
		offs = 0;
	}

	/* Get file's properties */
	res = phfs_stat(handle, &stat);
	if (res < 0) {
		log_error("\nCan't get stat from %s (%d)", kname, res);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	/* Check whether map's range coincides with device's address space */
	res = phfs_map(handle, offs, stat.size, mAttrRead | mAttrExec, tbeg, tsz, mAttrRead | mAttrExec, &addr);
	if (res < 0) {
		log_error("\nDevice is not mappable in %s (%d)", argv[1], res);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	if (res != dev_isMappable) {
		log_error("\n%s is not mappable in %s (%d)", kname, argv[1], res);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	entry = syspage_entryAdd(NULL, tbeg, tsz, sizeof(long long));
	if (entry == NULL) {
		log_error("\nCannot allocate memory for '%s'", kname);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	entry = syspage_entryAdd(NULL, dbeg, dsz, sizeof(long long));
	if (entry == NULL) {
		log_error("\nCannot allocate memory for '%s'", kname);
		phfs_close(handle);
		return CMD_EXIT_FAILURE;
	}

	hal_kernelEntryPoint(hal_kernelGetAddress(kentry));
	phfs_close(handle);

	log_info("\nLoaded %s", kname);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t kernelimg_cmd __attribute__((section("commands"), used)) = {
	.name = "kernelimg", .run = cmd_kernelimg, .info = cmd_kernelimgInfo
};
