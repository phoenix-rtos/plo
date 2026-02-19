/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Load application
 *
 * Copyright 2020-2021 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "elf.h"

#include <lib/lib.h>
#include <hal/hal.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_appInfo(void)
{
	lib_printf("loads app, usage: app [<dev> [-x | -xn] <name> <imap1;imap2...> <dmap1;dmap2...>]");
}


static int cmd_cp2ent(handler_t handler, const mapent_t *entry)
{
	ssize_t len;
	u8 buff[SIZE_MSG_BUFF];
	size_t offs, sz = entry->end - entry->start;

	for (offs = 0; offs < sz; offs += len) {
		if ((len = phfs_read(handler, offs, buff, min(SIZE_MSG_BUFF, sz - offs))) < 0) {
			log_error("\nCan't read data");
			return len;
		}
		hal_memcpy((void *)(entry->start + offs), buff, len);
	}

	return EOK;
}


static int cmd_mapsAdd2Prog(u8 *mapIDs, size_t nb, const char *mapNames)
{
	u8 id;
	int res, i;

	for (i = 0; i < nb; ++i) {
		if ((res = syspage_mapNameResolve(mapNames, &id)) < 0) {
			log_error("\nCan't add map %s", mapNames);
			return res;
		}

		mapIDs[i] = id;
		mapNames += hal_strlen(mapNames) + 1; /* name + '\0' */
	}

	return EOK;
}


static size_t cmd_mapsParse(char *maps, char sep)
{
	size_t nb = 0;

	while (*maps != '\0') {
		if (*maps == sep) {
			*maps = '\0';
			++nb;
		}
		maps++;
	}

	return ++nb;
}


static int cmd_partitionCreate(const char *name, syspage_part_t **part, char *imaps, size_t imapSz, char *dmaps, size_t dmapSz)
{
	int res;
	syspage_part_t *partition;

	if ((partition = syspage_partAdd()) == NULL ||
			(partition->allocMaps = syspage_alloc(sizeof(u8))) == NULL ||
			(partition->accessMaps = syspage_alloc((imapSz + dmapSz - 1) * sizeof(u8))) == NULL ||
			(partition->name = syspage_alloc(hal_strlen(name) + 1)) == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}
	cmd_mapsAdd2Prog(partition->allocMaps, 1, dmaps);
	cmd_mapsAdd2Prog(partition->accessMaps, dmapSz - 1, dmaps + (hal_strlen(dmaps) + 1));
	cmd_mapsAdd2Prog(partition->accessMaps, imapSz, imaps);
	partition->allocMapSz = 1;
	partition->accessMapSz = imapSz + dmapSz + 1;
	partition->availableMem = (unsigned long)-1;
	hal_strcpy(partition->name, name);

	partition->schedWindowsMask = 1U;

	if ((res = hal_getPartData(partition, imaps, imapSz, dmaps, dmapSz)) < 0) {
		return res;
	}
	*part = partition;

	return EOK;
}


static int cmd_appLoad(handler_t handler, size_t size, const char *name, char *imaps, char *dmaps, const char *appArgv, u32 flags, char *partName)
{
	int res;
	Elf32_Ehdr hdr;

	unsigned int attr;
	size_t dmapSz, imapSz;
	addr_t start, end, offs, addr;

	syspage_prog_t *prog;
	const mapent_t *entry;
	syspage_part_t *partition;

	/* Check ELF header */
	if ((res = phfs_read(handler, 0, &hdr, sizeof(Elf32_Ehdr))) < 0) {
		log_error("\nCan't read data");
		return res;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		log_error("\nFile isn't an ELF object");
		return -EIO;
	}

	/* First instance in imap is a map for the instructions */
	imapSz = cmd_mapsParse(imaps, ';');
	dmapSz = cmd_mapsParse(dmaps, ';');

	if (syspage_mapAttrResolve(imaps, &attr) < 0 ||
			syspage_mapRangeResolve(imaps, &start, &end) < 0) {
		log_error("\n%s does not exist", imaps);
		return -EINVAL;
	}

	if (phfs_aliasAddrResolve(handler, &offs) < 0)
		offs = 0;

	/* Check whether map's range coincides with device's address space */
	if ((res = phfs_map(handler, offs, size, mAttrRead | mAttrExec, start, end - start, attr, &addr)) < 0) {
		log_error("\nDevice is not mappable in %s", imaps);
		return res;
	}

	if (res == dev_isMappable || (res == dev_isNotMappable && (flags & flagSyspageNoCopy) != 0)) {
		if ((entry = syspage_entryAdd(NULL, addr + offs, size, SIZE_PAGE)) == NULL) {
			log_error("\nCannot allocate memory for %s", name);
			return -ENOMEM;
		}
	}
	else if (res == dev_isNotMappable) {
		if ((entry = syspage_entryAdd(imaps, (addr_t)-1, size, SIZE_PAGE)) == NULL) {
			log_error("\nCannot allocate memory for %s", name);
			return -ENOMEM;
		}

		/* Copy elf file to selected entry */
		if ((res = cmd_cp2ent(handler, entry)) < 0)
			return res;
	}
	else {
		log_error("\nDevice mappable routine failed");
		return -ENOMEM;
	}

	if (partName == NULL) {
		if ((res = cmd_partitionCreate(name, &partition, imaps, imapSz, dmaps, dmapSz)) != EOK) {
			return res;
		}
	}
	else if (syspage_partResolve(partName, &partition) != EOK) {
		log_error("\nPartition `%s` does not exist!", partName);
		return -EINVAL;
	}

	if ((prog = syspage_progAdd(appArgv, flags)) == NULL ||
			(prog->imaps = syspage_alloc(imapSz * sizeof(u8))) == NULL ||
			(prog->dmaps = syspage_alloc(dmapSz * sizeof(u8))) == NULL) {
		log_error("\nCannot allocate memory for %s", name);
		return -ENOMEM;
	}


	if ((res = cmd_mapsAdd2Prog(prog->imaps, imapSz, imaps)) < 0 ||
			(res = cmd_mapsAdd2Prog(prog->dmaps, dmapSz, dmaps)) < 0)
		return res;

	prog->imapSz = imapSz;
	prog->dmapSz = dmapSz;
	prog->start = entry->start;
	prog->end = entry->end;
	prog->partition = partition;

	return EOK;
}


static int cmd_app(int argc, char *argv[])
{
	size_t pos;
	int res, argvID = 0;

	char *imaps, *dmaps;
	unsigned int flags = 0;

	const char *appArgv;
	char name[SIZE_CMD_ARG_LINE];
	char *part;

	handler_t handler;
	phfs_stat_t stat;

	/* Parse command arguments */
	if (argc == 1) {
		syspage_progShow();
		return CMD_EXIT_SUCCESS;
	}
	else if (argc < 5 || argc > 7) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_0: command name */

	/* ARG_1: alias to device - it will be checked in phfs_open */

	/* ARG_2: optional flags */
	argvID = 2;
	if (argv[argvID][0] == '-') {
		if ((argv[argvID][1] | 0x20) == 'x' && argv[argvID][2] == '\0') {
			flags |= flagSyspageExec;
			argvID++;
		}
		else if ((argv[argvID][1] | 0x20) == 'x' && (argv[argvID][2] | 0x20) == 'n' && argv[argvID][3] == '\0') {
			flags |= flagSyspageExec | flagSyspageNoCopy;
			argvID++;
		}
		else {
			log_error("\n%s: Wrong arguments", argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}

	if (argvID > (argc - 3)) {
		log_error("\n%s: Invalid arg, 'dmap' is not declared", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* ARG_3: name + argv */
	appArgv = argv[argvID];
	for (pos = 0; appArgv[pos]; pos++) {
		if (appArgv[pos] == ';') {
			break;
		}
	}
	hal_memcpy(name, appArgv, pos);
	name[pos] = '\0';

	/* ARG_4: maps for instructions */
	imaps = argv[++argvID];

	/* ARG_5: maps for data */
	dmaps = argv[++argvID];

	/* ARG_6: partition name */
	if (argc > ++argvID) {
		part = argv[argvID];
	}
	else {
		part = NULL;
	}

	/* Open file */
	res = phfs_open(argv[1], name, 0, &handler);
	if (res < 0) {
		log_error("\nCan't open %s on %s (%d)", name, argv[1], res);
		return CMD_EXIT_FAILURE;
	}

	/* Get file's properties */
	res = phfs_stat(handler, &stat);
	if (res < 0) {
		log_error("\nCan't get stat from %s (%d)", name, res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	res = cmd_appLoad(handler, stat.size, name, imaps, dmaps, appArgv, flags, part);
	if (res < 0) {
		log_error("\nCan't load %s to %s via %s (%d)", name, imaps, argv[1], res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	log_info("\nLoaded %s", name);
	phfs_close(handler);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t app_cmd __attribute__((section("commands"), used)) = {
	.name = "app", .run = cmd_app, .info = cmd_appInfo
};
