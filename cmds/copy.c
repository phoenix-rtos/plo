/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Copy data between devices
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>
#include <phfs/phfs.h>


static void cmd_copyInfo(void)
{
	lib_printf("copies data between devices, usage:\n");
	lib_printf("%17s%s", "", "copy <src dev> <file/offs size> <dst dev> <file/offs size>");
}


static ssize_t cmd_cpphfs2phfs(handler_t srcHandler, addr_t srcAddr, size_t srcSz, handler_t dstHandler, addr_t dstAddr, size_t dstSz)
{
	ssize_t res;
	u8 buff[SIZE_MSG_BUFF];
	size_t chunk, rsz = 0, wsz = 0;

	/* Size is not defined, copy the whole file                 */
	if (srcSz == 0 && dstSz == 0)
		srcSz = -1;
	/* Size is defined, use smaller one to copy piece of memory */
	else if (srcSz != 0 && dstSz != 0)
		srcSz = (srcSz < dstSz) ? srcSz : dstSz;
	/* One of the size is not defined, use the defined one      */
	else
		srcSz = (srcSz > dstSz) ? srcSz : dstSz;

	do {
		chunk = ((srcSz - rsz) > sizeof(buff)) ? sizeof(buff) : (srcSz - rsz);
		if ((res = phfs_read(srcHandler, srcAddr + rsz, buff, chunk)) < 0) {
			log_error("\nCan't read data");
			return res;
		}
		rsz += res;

		chunk = res;
		while (chunk) {
			if ((res = phfs_write(dstHandler, dstAddr + wsz, buff, chunk)) < 0) {
				log_error("\nCan't write data to address: 0x%x", dstAddr + wsz);
				return res;
			}
			chunk -= res;
			wsz += res;
		}
	} while ((srcSz - rsz) > 0 && res > 0);

	return wsz;
}


static int cmd_devParse(handler_t *h, addr_t *offs, size_t *sz, unsigned int argc, char *argv[], int isDst, unsigned int *argvID, const char **file)
{
	int res;
	const char *alias;
	char *endptr;
	int flags = (isDst == 0) ? PHFS_OPEN_RDONLY : (PHFS_OPEN_CREATE | PHFS_OPEN_RDWR);

	if (h == NULL || offs == NULL || sz == NULL || argv == NULL || argvID == NULL)
		return -EINVAL;

	alias = argv[(*argvID)++];

	if (*argvID >= argc) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	*offs = lib_strtoul(argv[*argvID], &endptr, 0);

	/* Open device using alias to file */
	if (*endptr) {
		*offs = 0;
		*sz = 0;
		*file = argv[*argvID];
		if ((res = phfs_open(alias, argv[*argvID], flags, h)) < 0) {
			log_error("\nCan't open file '%s' on %s", argv[*argvID], alias);
			return res;
		}
	}
	/* Open device using direct access to memory */
	else {
		*sz = lib_strtoul(argv[++(*argvID)], &endptr, 0);
		*file = NULL;
		if (*endptr) {
			log_error("\nWrong size value: %s, for %s with offs 0x%x", argv[(*argvID)], alias, *offs);
			return -EINVAL;
		}

		if ((res = phfs_open(alias, NULL, 0, h)) < 0) {
			log_error("\nCan't open file '%s' with offset 0x%x", alias, *offs);
			return res;
		}
	}
	(*argvID)++;

	return EOK;
}


static int cmd_copy(int argc, char *argv[])
{
	size_t sz[2];
	ssize_t res;
	addr_t offs[2];
	handler_t h[2];
	const char *file[2];

	unsigned int argvID = 1;

	/* Parse all comand's arguments */
	if (argc < 5) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	if ((res = cmd_devParse(&h[0], &offs[0], &sz[0], argc, argv, 0, &argvID, &file[0])) < 0)
		return res;

	if ((res = cmd_devParse(&h[1], &offs[1], &sz[1], argc, argv, 1, &argvID, &file[1])) < 0) {
		phfs_close(h[0]);
		return res;
	}

	/* Copy data between devices */
	log_info("\nCopying data, please wait...");
	res = cmd_cpphfs2phfs(h[0], offs[0], sz[0], h[1], offs[1], sz[1]);

	phfs_close(h[0]);
	phfs_close(h[1]);

	if (res < 0) {
		log_error("\nCopying failed");
		return res;
	}

	log_info("\nFinished copying");

	if ((res = phfs_aliasReg((file[1] == NULL) ? file[0] : file[1], offs[1], res)) < 0)
		return res;

	return EOK;
}


__attribute__((constructor)) static void cmd_copyReg(void)
{
	const static cmd_t app_cmd = { .name = "copy", .run = cmd_copy, .info = cmd_copyInfo };

	cmd_reg(&app_cmd);
}
