/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * copy command
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
	int res;
	addr_t addr = dstAddr;
	size_t size, chunk;
	u8 buff[SIZE_MSG_BUFF];

	/* Size is not defined, copy the whole file                 */
	if (srcSz == 0 && dstSz == 0)
		size = -1;
	/* Size is defined, use smaller one to copy piece of memory */
	else if (srcSz != 0 && dstSz != 0)
		size = (srcSz < dstSz) ? srcSz : dstSz;
	/* One of the size is not defined, use the defined one      */
	else
		size = (srcSz > dstSz) ? srcSz : dstSz;

	do {
		if (size > sizeof(buff))
			chunk = sizeof(buff);
		else
			chunk = size;

		if ((res = phfs_read(srcHandler, srcAddr, buff, chunk)) < 0) {
			log_error("\nCan't read data\n");
			return -EIO;
		}
		srcAddr += res;
		size -= res;

		if ((res = phfs_write(dstHandler, addr, buff, res)) < 0) {
			log_error("\nCan't write data to address: 0x%x\n", addr);
			return -EIO;
		}
		addr += res;
	} while (size > 0 && res > 0);

	return addr - dstAddr;
}


static int cmd_parseDev(handler_t *h, addr_t *offs, size_t *sz, cmdarg_t *args, unsigned int *argsID, unsigned int argsc, const char **file)
{
	char *alias;
	char *endptr;

	if (h == NULL || offs == NULL || sz == NULL || args == NULL || argsID == NULL)
		return -EINVAL;

	alias = args[(*argsID)++];

	if (*argsID >= argsc) {
		log_error("\nWrong args for %s", alias);
		return -EINVAL;
	}

	*offs = lib_strtoul(args[*argsID], &endptr, 0);

	/* Open device using alias to file */
	if (*endptr) {
		*offs = 0;
		*sz = 0;
		*file = args[*argsID];
		if (phfs_open(alias, args[*argsID], 0, h) < 0) {
			log_error("\nCan't open file '%s' on %s", args[*argsID], alias);
			return -EIO;
		}
		(*argsID)++;
	}
	/* Open device using direct access to memory */
	else {
		*sz = lib_strtoul(args[++(*argsID)], &endptr, 0);
		*file = NULL;
		if (*endptr) {
			log_error("\nWrong size value: %s, for %s with offs 0x%x", args[(*argsID)], alias, *offs);
			return -EINVAL;
		}

		if (phfs_open(alias, NULL, 0, h) < 0) {
			log_error("\nCan't open file '%s' with offset 0x%x", alias, *offs);
			return -EIO;
		}
	}

	return EOK;
}


static int cmd_copy(char *s)
{
	size_t sz[2];
	ssize_t res;
	addr_t offs[2];
	handler_t h[2];
	const char *file[2];

	unsigned int argsc, argsID = 0;
	cmdarg_t *args;

	/* Parse all comand's arguments */
	if ((argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args)) < 4) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	if (cmd_parseDev(&h[0], &offs[0], &sz[0], args, &argsID, argsc, &file[0]) < 0)
		return -EINVAL;

	if (cmd_parseDev(&h[1], &offs[1], &sz[1], args, &argsID, argsc, &file[1]) < 0)
		return -EINVAL;

	/* Copy data between devices */
	log_info("\nCopying data, please wait...");
	if ((res = cmd_cpphfs2phfs(h[0], offs[0], sz[0], h[1], offs[1], sz[1])) < 0) {
		log_error("\nCopying failed");
		phfs_close(h[0]);
		phfs_close(h[1]);
		return -EINVAL;
	}

	log_info("\nFinished copying");

	phfs_close(h[0]);
	phfs_close(h[1]);

	if (phfs_regFile((file[1] == NULL) ? file[0] : file[1], offs[1], res) < 0)
		return -ENXIO;

	return EOK;
}


__attribute__((constructor)) static void cmd_copyReg(void)
{
	const static cmd_t app_cmd = { .name = "copy", .run = cmd_copy, .info = cmd_copyInfo };

	cmd_reg(&app_cmd);
}
