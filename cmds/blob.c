/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Load application
 *
 * Copyright 2020-2021, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <lib/lib.h>
#include <hal/hal.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_blobInfo(void)
{
	lib_printf("put file in the syspage, usage: blob [<dev> <name> <map>]");
}

static int cmd_cp2ent(handler_t handler, const mapent_t *entry)
{
	ssize_t len;
	u8 buff[SIZE_MSG_BUFF];
	size_t offs, sz = entry->end - entry->start;

	for (offs = 0; offs < sz; offs += len) {
		len = phfs_read(handler, offs, buff, min(SIZE_MSG_BUFF, sz - offs));
		if (len < 0) {
			log_error("\nCan't read data");
			return len;
		}
		hal_memcpy((void *)(entry->start + offs), buff, len);
	}

	return EOK;
}

static int cmd_blobLoad(handler_t handler, size_t size, const char *name, const char *map)
{
	int res;

	unsigned int attr;
	addr_t start, end, offs, addr;

	syspage_prog_t *prog;
	const mapent_t *entry;

	if (phfs_aliasAddrResolve(handler, &offs) < 0) {
		offs = 0;
	}

	if (syspage_mapAttrResolve(map, &attr) < 0 ||
			syspage_mapRangeResolve(map, &start, &end) < 0) {
		log_error("\n%s does not exist", map);
		return -EINVAL;
	}

	/* Check whether map's range coincides with device's address space */
	res = phfs_map(handler, offs, size, mAttrRead, start, end - start, attr, &addr);
	if (res < 0) {
		log_error("\nDevice is not mappable in %s", map);
		return res;
	}

	if (res == dev_isMappable) {
		entry = syspage_entryAdd(NULL, addr + offs, size, SIZE_PAGE);
		if (entry == NULL) {
			log_error("\nCannot allocate memory for %s", name);
			return -ENOMEM;
		}
	}
	else if (res == dev_isNotMappable) {
		entry = syspage_entryAdd(map, (addr_t)-1, size, SIZE_PAGE);
		if (entry == NULL) {
			log_error("\nCannot allocate memory for %s", name);
			return -ENOMEM;
		}

		/* Copy file to the selected entry */
		res = cmd_cp2ent(handler, entry);
		if (res < 0) {
			return res;
		}
	}
	else {
		log_error("\nDevice mappable routine failed");
		return -ENOMEM;
	}

	prog = syspage_progAdd(name, 0);
	if (prog == NULL) {
		log_error("\nCannot add syspage program for %s", name);
	}

	prog->imaps = NULL;
	prog->imapSz = 0;
	prog->dmaps = NULL;
	prog->dmapSz = 0;
	prog->start = entry->start;
	prog->end = entry->end;

	return EOK;
}


static int cmd_blob(int argc, char *argv[])
{
	int res;
	const char *dev;
	const char *name;
	const char *map;

	handler_t handler;
	phfs_stat_t stat;

	/* Parse command arguments */
	if (argc == 1) {
		syspage_progShow();
		return CMD_EXIT_SUCCESS;
	}
	if (argc != 4) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	dev = argv[1];
	name = argv[2];
	map = argv[3];

	/* Open file */
	res = phfs_open(dev, name, 0, &handler);
	if (res < 0) {
		log_error("\nCan't open %s on %s (%d)", name, dev, res);
		return CMD_EXIT_FAILURE;
	}

	/* Get file's properties */
	res = phfs_stat(handler, &stat);
	if (res < 0) {
		log_error("\nCan't get stat from %s (%d)", name, res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	res = cmd_blobLoad(handler, stat.size, name, map);
	if (res < 0) {
		log_error("\nCan't load %s to %s via %s (%d)", name, map, dev, res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	log_info("\nLoaded %s", name);
	phfs_close(handler);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t blob_cmd __attribute__((section("commands"), used)) = {
	.name = "blob", .run = cmd_blob, .info = cmd_blobInfo
};
