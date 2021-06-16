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
#include "elf.h"

#include <hal/hal.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_appInfo(void)
{
	lib_printf("loads app, usage: app [<dev> [-x] <name> <imap> <dmap>]");
}


static int cmd_cpphfs2map(handler_t handler, const char *imap, addr_t top)
{
	ssize_t len, res;
	addr_t offs = 0;
	u8 buff[SIZE_MSG_BUFF];

	do {
		if ((len = phfs_read(handler, offs, buff, SIZE_MSG_BUFF)) < 0) {
			syspage_setMapTop(imap, top);
			log_error("\nCan't read data");
			return len;
		}

		offs += len;

		if ((res = syspage_write2Map(imap, buff, len)) < 0) {
			syspage_setMapTop(imap, top);
			return res;
		}
	} while (len > 0);

	return EOK;
}


static int cmd_loadApp(handler_t handler, size_t size, const char *imap, const char *dmap, const char *cmdline, u32 flags)
{
	size_t res;
	Elf32_Ehdr hdr;
	addr_t start, end;
	addr_t addr, offs = 0;
	unsigned int attr;

	/* Check ELF header */
	if ((res = phfs_read(handler, offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr))) < 0) {
		log_error("\nCan't read data");
		return res;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		log_error("\nFile isn't an ELF object");
		return -EIO;
	}

	/* Align map top, so the app begin is aligned */
	if (syspage_alignMapTop(imap) < 0)
		return -EINVAL;

	/* Get top address of map and its attributes */
	if (syspage_getMapTop(imap, &start) < 0 || syspage_getMapAttr(imap, &attr) < 0) {
		log_error("\n%s does not exist", imap);
		return -EINVAL;
	}

	if ((res = phfs_map(handler, offs, size, mAttrRead | mAttrWrite, start, size, attr, &addr)) < 0) {
		log_error("\nDevice is not mappable in %s", imap);
		return res;
	}

	/* Copy program's elf to imap */
	if (res == dev_isNotMappable) {
		if ((res = cmd_cpphfs2map(handler, imap, start)) < 0)
			return res;

		/* Get map top address after copying */
		syspage_getMapTop(imap, &end);
	}
	else if (res == dev_isMappable) {
		if ((res = phfs_getFileAddr(handler, &offs)) < 0)
			return res;

		start = offs + addr;
		end = start + size;
	}
	else {
		log_error("\nDevice returns wrong mapping result.");
		return -EIO;
	}

	if ((res = syspage_addProg(start, end, imap, dmap, cmdline, flags)) < 0) {
		log_error("\nCan't add program to syspage");
		return res;
	}

	return EOK;
}


/* TODO: allow to add more than 2 maps */
static int cmd_app(int argc, char *argv[])
{
	int res, argvID = 0;
	unsigned int pos;

	const char *cmdline;
	unsigned int flags = 0;
	char imap[8], dmap[8], appName[SIZE_APP_NAME + 1];

	handler_t handler;
	phfs_stat_t stat;

	/* Parse command arguments */
	if (argc == 1) {
		syspage_showApps();
		return EOK;
	}
	else if (argc < 5 || argc > 6) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	/* ARG_0: alias to device - it will be check in phfs_open */

	/* ARG_1: optional flags */
	argvID = 2;
	if (argv[argvID][0] == '-') {
		if ((argv[argvID][1] | 0x20) == 'x' && argv[argvID][2] == '\0') {
			flags |= flagSyspageExec;
			argvID++;
		}
		else {
			log_error("\n%s: Wrong arguments", argv[0]);
			return -EINVAL;
		}
	}

	/* ARG_2: cmdline */
	if (argvID != (argc - 3)) {
		log_error("\n%s: Invalid arg, 'dmap' is not declared", argv[0]);
		return -EINVAL;
	}

	/* Get app name from cmdline */
	cmdline = argv[argvID];
	for (pos = 0; cmdline[pos]; pos++) {
		if (cmdline[pos] == ';')
			break;
	}

	if (pos > SIZE_APP_NAME) {
		log_error("\nApp %s name is too long", cmdline);
		return -EINVAL;
	}

	hal_memcpy(appName, argv[argvID], pos);
	appName[pos] = '\0';

	/* ARG_3: Get map for instruction section */
	hal_memcpy(imap, argv[++argvID], sizeof(imap));
	imap[sizeof(imap) - 1] = '\0';

	/* ARG_4: Get map for data section */
	hal_memcpy(dmap, argv[++argvID], sizeof(dmap));
	dmap[sizeof(dmap) - 1] = '\0';

	/* Open file */
	if ((res = phfs_open(argv[1], appName, 0, &handler)) < 0) {
		log_error("\nCan't open %s on %s", appName, argv[1]);
		return res;
	}

	/* Get file's properties */
	if ((res = phfs_stat(handler, &stat)) < 0) {
		log_error("\nCan't get stat from %s", argv[1]);
		return res;
	}

	if ((res = cmd_loadApp(handler, stat.size, imap, dmap, cmdline, flags)) < 0) {
		log_error("\nCan't load %s to %s via %s", appName, imap, argv[1]);
		return res;
	}

	if ((res = phfs_close(handler)) < 0) {
		log_error("\nCan't  close %s", appName);
		return res;
	}

	log_info("\nLoading %s", appName);

	return EOK;
}


__attribute__((constructor)) static void cmd_appreg(void)
{
	const static cmd_t app_cmd = { .name = "app", .run = cmd_app, .info = cmd_appInfo };

	cmd_reg(&app_cmd);
}
