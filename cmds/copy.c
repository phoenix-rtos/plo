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
#include "lib.h"
#include "phfs.h"


static void cmd_copyInfo(void)
{
	lib_printf("copies data between devices, usage:\n");
	lib_printf("%17s%s", "", "copy <src device> <src file/LBA> <dst device> <dst file/LBA> [<len>]");
}


static int cmd_cpphfs2phfs(handler_t srcHandler, addr_t srcAddr, size_t srcSz, handler_t dstHandler, addr_t dstAddr, size_t dstSz)
{
	int res;
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
			lib_printf("\nCan't read data\n");
			return ERR_PHFS_FILE;
		}
		srcAddr += res;
		size -= res;

		if ((res = phfs_write(dstHandler, dstAddr, buff, res)) < 0) {
			lib_printf("\nCan't write data!\n");
			return ERR_PHFS_FILE;
		}
		dstAddr += res;
	} while (size > 0 && res > 0);

	return 0;
}


static int cmd_parseDev(handler_t *h, addr_t *offs, size_t *sz, char (*args)[SIZE_CMD_ARG_LINE + 1], u16 *argsID, u16 argsc)
{
	char *alias;

	if (h == NULL || offs == NULL || sz == NULL || args == NULL || argsID == NULL)
		return ERR_ARG;

	alias = args[(*argsID)++];

	if ((*argsID) >= argsc) {
		lib_printf("\nWrong number of arguments\n");
		return ERR_ARG;
	}

	/* Open device using alias to file */
	if (lib_ishex(args[(*argsID)]) < 0) {
		*offs = 0;
		*sz = 0;
		if (phfs_open(alias, args[(*argsID)++], 0, h) < 0) {
			lib_printf("\nCannot initialize source: %s\n", alias);
			return ERR_PHFS_IO;
		}
	}
	/* Open device using direct access to memory */
	else {
		/* check whether size is provided */
		if (((*argsID) + 1) >= argsc || lib_ishex(args[(*argsID) + 1]) < 0) {
			lib_printf("\nWrong number of arguments\n");
			return ERR_ARG;
		}

		*offs = lib_strtoul(args[(*argsID)], NULL, 16);
		*sz = lib_strtoul(args[++(*argsID)], NULL, 16);

		if (phfs_open(alias, NULL, 0, h) < 0) {
			lib_printf("\nCannot initialize source: %s\n", alias);
			return ERR_PHFS_IO;
		}
	}

	return ERR_NONE;
}


static int cmd_copy(char *s)
{
	size_t sz[2];
	addr_t offs[2];
	handler_t h[2];

	u16 argsc = 0, argsID = 0;
	char args[6][SIZE_CMD_ARG_LINE + 1];

	/* Parse all comand's arguments */
	if (cmd_parseArgs(s, args, &argsc, 6) < 0 || argsc < 4) {
		lib_printf("\nWrong arguments!!\n");
		return ERR_ARG;
	}

	if (cmd_parseDev(&h[0], &offs[0], &sz[0], args, &argsID, argsc) < 0) {
		lib_printf("\nCannot open file\n");
		return ERR_ARG;
	}

	if (cmd_parseDev(&h[1], &offs[1], &sz[1], args, &argsID, argsc) < 0) {
		lib_printf("\nCannot open file\n");
		return ERR_ARG;
	}

	/* Copy data between devices */
	if (cmd_cpphfs2phfs(h[0], offs[0], sz[0], h[1], offs[1], sz[1]) < 0) {
		lib_printf("\nOperation failed\n");
		phfs_close(h[0]);
		phfs_close(h[1]);
		return ERR_ARG;
	}

	lib_printf("\nFinished copying\n");

	phfs_close(h[0]);
	phfs_close(h[1]);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_copyReg(void)
{
	const static cmd_t app_cmd = { .name = "copy", .run = cmd_copy, .info = cmd_copyInfo };

	cmd_reg(&app_cmd);
}
