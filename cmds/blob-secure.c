/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Load secure application
 *
 * Copyright 2020-2021, 2024 2025 Phoenix Systems
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

#include <hal/armv8m/stm32/n6/pka.h>
#include <hal/armv8m/stm32/n6/hash.h>

#define HASH_ALGO_STR "sha2-256"
#define HASH_BYTES    32


static void cmd_blobsecureInfo(void)
{
	lib_printf("put file hash and puts it in the syspage, usage: blob-secure [<dev> <name> <map>]");
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
			log_error("\nInvalid blob hash");
			return -EINVAL;
		}
	}

	return EOK;
}

static int cmd_blobLoadSecure(handler_t handler, size_t size, const char *name, const char *map, u8 *hash)
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
		/* File is not copied, so verification must be done in the kernel. Checking the hash here is optional */
		res = cmd_checkHash(handler, size, hash);
		if (res < 0) {
			log_error("\nInvalid hash for %d", name);
			return -EINVAL;
		}
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

		/* Copy file to selected entry. Check the hash of the copied file */
		res = cmd_cp2entSecure(handler, entry, hash);
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


static int cmd_blobsecure(int argc, char *argv[])
{
	int res;
	const char *dev;
	const char *name;
	const char *map;
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
	if (argc != 6) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	dev = argv[1];
	name = argv[2];
	map = argv[3];
	hashAlgo = argv[4];
	argvHash = argv[5];

	if (hal_strcmp(hashAlgo, HASH_ALGO_STR) != 0) {
		log_error("\nHash algorithm: %s not supported.", argv[4]);
		return CMD_EXIT_FAILURE;
	}
	res = hash_base64_decode(argvHash, hashb64len, hash, HASH_BYTES);
	if (res < 0) {
		log_error("\nInvalid base64 format %s", argv[5]);
		return CMD_EXIT_SUCCESS;
	}

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

	res = cmd_blobLoadSecure(handler, stat.size, name, map, hash);
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
	.name = "blob-secure", .run = cmd_blobsecure, .info = cmd_blobsecureInfo
};
