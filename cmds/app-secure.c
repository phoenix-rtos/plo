/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Load secure application
 *
 * Copyright 2020-2021 2026 Phoenix Systems
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

#include <hal/armv8m/stm32/n6/pka.h>
#include <hal/armv8m/stm32/n6/hash.h>

#define HASH_ALGO_STR "sha2-256"
#define HASH_BYTES    32


static void cmd_appsecureInfo(void)
{
	lib_printf("checks app hash and loads it, usage: app-secure [<dev> [-x | -xn] <name> <imap1;imap2...> <dmap1;dmap2...> <hash_algorithm> <app_hash>]");
}


static int cmd_checkHash(handler_t handler, size_t filesz, u8 *expected)
{
	ssize_t len;
	u8 buff[SIZE_MSG_BUFF];
	u8 calcHash[HASH_BYTES];
	size_t offs;

	(void)hash_initDigest(HASH_ALGO_SHA2_256);

	for (offs = 0; offs < filesz; offs += len) {
		if ((len = phfs_read(handler, offs, buff, min(SIZE_MSG_BUFF, filesz - offs))) < 0) {
			log_error("\nCan't read data");
			return len;
		}
		hash_feedMessage(buff, len);
	}
	hash_getDigest(calcHash, HASH_BYTES);

	for (offs = 0; offs < HASH_BYTES; offs++) {
		if (calcHash[offs] != expected[offs]) {
			return -EINVAL;
		}
	}

	return EOK;
}


static int cmd_cp2entSecure(handler_t handler, const mapent_t *entry, u8 *expected)
{
	ssize_t len;
	u8 buff[SIZE_MSG_BUFF];
	size_t offs, sz = entry->end - entry->start;
	u8 calcHash[HASH_BYTES];

	(void)hash_initDigest(HASH_ALGO_SHA2_256);

	for (offs = 0; offs < sz; offs += len) {
		if ((len = phfs_read(handler, offs, buff, min(SIZE_MSG_BUFF, sz - offs))) < 0) {
			log_error("\nCan't read data");
			return len;
		}
		hal_memcpy((void *)(entry->start + offs), buff, len);
		hash_feedMessage(buff, len);
	}

	hash_getDigest(calcHash, HASH_BYTES);

	for (offs = 0; offs < HASH_BYTES; offs++) {
		if (calcHash[offs] != expected[offs]) {
			log_error("\nInvalid app hash");
			return -EINVAL;
		}
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


static int cmd_appLoadSecure(handler_t handler, size_t size, const char *name, char *imaps, char *dmaps, const char *appArgv, u32 flags, u8 *hash)
{
	int res;
	Elf32_Ehdr hdr;

	unsigned int attr;
	size_t dmapSz, imapSz;
	addr_t start, end, offs, addr;

	syspage_prog_t *prog;
	const mapent_t *entry;


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
		/* Program is not copied, so verification must be done in the kernel. Checking the hash here is optional */
		res = cmd_checkHash(handler, size, hash);
		if (res < 0) {
			log_error("\nInvalid hash for %d", name);
			return -EINVAL;
		}

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

		/* Copy elf file to selected entry. Check the hash of the copied file */
		if ((res = cmd_cp2entSecure(handler, entry, hash)) < 0)
			return res;
	}
	else {
		log_error("\nDevice mappable routine failed");
		return -ENOMEM;
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

	return EOK;
}


static int cmd_appsecure(int argc, char *argv[])
{
	size_t pos;
	int res, argvID = 0;

	char *imaps, *dmaps;
	unsigned int flags = 0;

	const char *appArgv;
	char name[SIZE_CMD_ARG_LINE];
	const char *hashAlgo;
	const char *argvHash;
	u8 hash[HASH_BYTES];
	u32 hashb64len = ((HASH_BYTES + 3 - 1) / 3) * 4;

	handler_t handler;
	phfs_stat_t stat;

	/* Parse command arguments */
	if (argc == 1) {
		syspage_progShow();
		return CMD_EXIT_SUCCESS;
	}
	else if (argc < 7 || argc > 8) {
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

	if (argvID != (argc - 5)) {
		log_error("\n%s: Invalid arg, '<app_hash>' is not declared", argv[0]);
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

	/* ARG_6: hash algorithm */
	hashAlgo = argv[++argvID];
	if (hal_strcmp(hashAlgo, HASH_ALGO_STR) != 0) {
		log_error("\nHash algorithm: %s not supported.", argv[4]);
		return CMD_EXIT_FAILURE;
	}
	argvHash = argv[++argvID];
	res = hash_base64_decode(argvHash, hashb64len, hash, HASH_BYTES);
	if (res < 0) {
		log_error("\nInvalid base64 format %s", argv[5]);
		return CMD_EXIT_SUCCESS;
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

	res = cmd_appLoadSecure(handler, stat.size, name, imaps, dmaps, appArgv, flags, hash);
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
	.name = "app-secure", .run = cmd_appsecure, .info = cmd_appsecureInfo
};
