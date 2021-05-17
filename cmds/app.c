/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Load application command
 *
 * Copyright 2020-2021 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "hal.h"
#include "elf.h"
#include "phfs.h"
#include "syspage.h"


static void cmd_appInfo(void)
{
	lib_printf("loads app, usage: app [<boot device>] [-x] <name> [imap] [dmap]");
}


static int cmd_cpphfs2map(handler_t handler, size_t size, const char *imap)
{
	int res;
	addr_t offs = 0;
	size_t chunkSz;
	u8 buff[SIZE_MSG_BUFF];

	do {
		if (size > sizeof(buff))
			chunkSz = sizeof(buff);
		else
			chunkSz = size;

		if ((res = phfs_read(handler, offs, buff, chunkSz)) < 0) {
			lib_printf("\nCan't read segment data\n");
			return ERR_PHFS_FILE;
		}

		if (syspage_write2Map(imap, buff, res) < 0)
			return ERR_ARG;

		offs += res;
		size -= res;
	} while (size > 0 && res > 0);

	return ERR_NONE;
}


static int cmd_loadApp(handler_t handler, size_t size, const char *imap, const char *dmap, const char *cmdline, u32 flags)
{
	int res;
	Elf32_Ehdr hdr;
	void *start, *end;
	addr_t addr, offs = 0;
	unsigned int attr;

	/* Check ELF header */
	if ((res = phfs_read(handler, offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr))) < 0) {
		lib_printf("\nCan't read ELF header %d\n", res);
		return ERR_PHFS_FILE;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		lib_printf("\nFile isn't ELF object\n");
		return ERR_PHFS_FILE;
	}

	/* Align map top, so the app begin is aligned */
	if (syspage_alignMapTop(imap) < 0)
		return ERR_ARG;

	/* Get top address of map and its attributes */
	if (syspage_getMapTop(imap, &start) < 0 || syspage_getMapAttr(imap, &attr) < 0) {
		lib_printf("\n%s does not exist!\n", imap);
		return ERR_ARG;
	}

	if ((res = phfs_map(handler, offs, size, mAttrRead | mAttrWrite, (addr_t)start, size, attr, &addr)) < 0) {
		lib_printf("\nDevice is not mappable in %s\n", imap);
		return ERR_ARG;
	}

	/* Copy program's elf to imap */
	if (res == dev_isNotMappable) {
		if ((res = cmd_cpphfs2map(handler, size, imap)) < 0)
			return res;

		/* Get map top address after copying */
		syspage_getMapTop(imap, &end);
	}
	else if (res == dev_isMappable) {
		if (phfs_getFileAddr(handler, &offs) < 0)
			return ERR_ARG;

		start = (void *)(offs + addr);
		end = start + size;
	}
	else {
		lib_printf("\nDevice returns wrong mapping result.");
		return ERR_PHFS_FILE;
	}

	if (syspage_addProg(start, end, imap, dmap, cmdline, flags) < 0) {
		lib_printf("\nCannot add program to syspage\n");
		return ERR_ARG;
	}

	return ERR_NONE;
}


/* TODO: allow to add more than 2 maps */
static int cmd_app(char *s)
{
	int argID = 0;
	u16 cmdArgsc = 0;
	char cmdArgs[10][SIZE_CMD_ARG_LINE + 1];

	char *cmdline;
	unsigned int pos = 0, flags = 0;
	char cmap[8], dmap[8], appName[SIZE_APP_NAME];

	size_t sz = 0;
	handler_t handler;
	phfs_stat_t stat;

	/* Parse command arguments */
	if (cmd_parseArgs(s, cmdArgs, &cmdArgsc, 10) < 0 || cmdArgsc < 2 || cmdArgsc > 6) {
		if (cmdArgsc == 0) {
			syspage_showApps();
			return ERR_NONE;
		}

		lib_printf("\nWrong arguments!!\n");
		return ERR_ARG;
	}

	/* ARG_0: alias to device - it will be check in phfs_open */

	/* ARG_1: optional flags */
	argID = 1;
	if (cmdArgs[argID][0] == '-') {
		if ((cmdArgs[argID][1] | 0x20) == 'x' && cmdArgs[argID][2] == '\0') {
			flags |= flagSyspageExec;
			argID++;
		}
		else {
			lib_printf("\nWrong arguments!!\n");
			return ERR_ARG;
		}
	}

	/* ARG_2: cmdline */
	if (argID >= cmdArgsc) {
		lib_printf("\nWrong arguments!!\n");
		return ERR_ARG;
	}

	/* Get app name from cmdline */
	cmdline = cmdArgs[argID];
	for (pos = 0; cmdline[pos]; pos++) {
		if (cmdline[pos] == ';')
			break;
	}

	if (pos > SIZE_APP_NAME) {
		lib_printf("\nApp %s name is too long!\n", cmdline);
		return ERR_ARG;
	}

	hal_memcpy(appName, cmdArgs[argID], pos);
	appName[pos] = '\0';

	/* ARG_3: Get map for instruction section */
	if ((argID + 1) < cmdArgsc) {
		hal_memcpy(cmap, cmdArgs[++argID], 8);
		cmap[sizeof(cmap) - 1] = '\0';
	}
	else {
		lib_printf("\nMap for instructions is not defined");
		return ERR_ARG;
	}

	/* ARG_4: Get map for data section */
	if ((argID + 1) < cmdArgsc) {
		hal_memcpy(dmap, cmdArgs[++argID], 8);
		dmap[sizeof(dmap) - 1] = '\0';
	}
	else {
		lib_printf("\nMap for data is not defined");
		return ERR_ARG;
	}

	/* Open file */
	if (phfs_open(cmdArgs[0], appName, 0, &handler) < 0) {
		lib_printf("\nWrong arguments!!\n");
		return ERR_ARG;
	}

	/* Get file's properties */
	if (phfs_stat(handler, &stat) < 0) {
		lib_printf("\nCannot get stat from file %s\n", cmdArgs[0]);
		return ERR_ARG;
	}
	sz = stat.size;

	cmd_loadApp(handler, sz, cmap, dmap, cmdline, flags);

	if (phfs_close(handler) < 0) {
		lib_printf("\nCannot close file\n");
		return ERR_ARG;
	}

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_appreg(void)
{
	const static cmd_t app_cmd = { .name = "app", .run = cmd_app, .info = cmd_appInfo };

	cmd_reg(&app_cmd);
}
