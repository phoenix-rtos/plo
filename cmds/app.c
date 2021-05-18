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
			log_error("\nCan't read data");
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
		log_error("\nCan't read data");
		return ERR_PHFS_FILE;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		log_error("\nFile isn't an ELF object");
		return ERR_PHFS_FILE;
	}

	/* Align map top, so the app begin is aligned */
	if (syspage_alignMapTop(imap) < 0)
		return ERR_ARG;

	/* Get top address of map and its attributes */
	if (syspage_getMapTop(imap, &start) < 0 || syspage_getMapAttr(imap, &attr) < 0) {
		log_error("\n%s does not exist", imap);
		return ERR_ARG;
	}

	if ((res = phfs_map(handler, offs, size, mAttrRead | mAttrWrite, (addr_t)start, size, attr, &addr)) < 0) {
		log_error("\nDevice is not mappable in %s", imap);
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
		log_error("\nDevice returns wrong mapping result.");
		return ERR_PHFS_FILE;
	}

	if (syspage_addProg(start, end, imap, dmap, cmdline, flags) < 0) {
		log_error("\nCan't add program to syspage");
		return ERR_ARG;
	}

	return ERR_NONE;
}


/* TODO: allow to add more than 2 maps */
static int cmd_app(char *s)
{
	int argID = 0;
	unsigned int argsc, pos;
	char (*args)[SIZE_CMD_ARG_LINE];

	char *cmdline;
	unsigned int flags = 0;
	char imap[8], dmap[8], appName[SIZE_APP_NAME];

	handler_t handler;
	phfs_stat_t stat;

	/* Parse command arguments */
	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	if (argsc == 0) {
		syspage_showApps();
		return ERR_NONE;
	}

	if (argsc < 4 || argsc > 6) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	/* ARG_0: alias to device - it will be check in phfs_open */

	/* ARG_1: optional flags */
	argID = 1;
	if (args[argID][0] == '-') {
		if ((args[argID][1] | 0x20) == 'x' && args[argID][2] == '\0') {
			flags |= flagSyspageExec;
			argID++;
		}
		else {
			log_error("\nWrong args: %s", s);
			return ERR_ARG;
		}
	}

	/* ARG_2: cmdline */
	if (argID >= argsc) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	/* Get app name from cmdline */
	cmdline = args[argID];
	for (pos = 0; cmdline[pos]; pos++) {
		if (cmdline[pos] == ';')
			break;
	}

	if (pos > SIZE_APP_NAME) {
		log_error("\nApp %s name is too long", cmdline);
		return ERR_ARG;
	}

	hal_memcpy(appName, args[argID], pos);
	appName[pos] = '\0';

	/* ARG_3: Get map for instruction section */
	if ((argID + 1) < argsc) {
		hal_memcpy(imap, args[++argID], 8);
		imap[sizeof(imap) - 1] = '\0';
	}
	else {
		log_error("\nInstruction's map for %s is not defined", appName);
		return ERR_ARG;
	}

	/* ARG_4: Get map for data section */
	if ((argID + 1) < argsc) {
		hal_memcpy(dmap, args[++argID], 8);
		dmap[sizeof(dmap) - 1] = '\0';
	}
	else {
		log_error("\nData's map for %s is not defined", appName);
		return ERR_ARG;
	}

	/* Open file */
	if (phfs_open(args[0], appName, 0, &handler) < 0) {
		log_error("\nCan't open %s on %s", appName, args[0]);
		return ERR_ARG;
	}

	/* Get file's properties */
	if (phfs_stat(handler, &stat) < 0) {
		log_error("\nCan't get stat from %s", args[0]);
		return ERR_ARG;
	}

	if (cmd_loadApp(handler, stat.size, imap, dmap, cmdline, flags) < 0) {
		log_error("\nCan't load %s to %s via %s", appName, imap, args[0]);
		return ERR_PHFS_IO;
	}

	if (phfs_close(handler) < 0) {
		log_error("\nCan't  close %s", appName);
		return ERR_ARG;
	}

	log_info("\nLoading %s", appName);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_appreg(void)
{
	const static cmd_t app_cmd = { .name = "app", .run = cmd_app, .info = cmd_appInfo };

	cmd_reg(&app_cmd);
}
