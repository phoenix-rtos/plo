/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Loader commands
 *
 * Copyright 2012, 2017, 2020-2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Pawel Kolodziej, Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "errors.h"
#include "syspage.h"
#include "hal.h"
#include "lib.h"
#include "console.h"
#include "phfs.h"
#include "elf.h"
#include "cmd.h"
#include "script.h"

#define MSG_BUFF_SZ 0x100


const cmd_t genericCmds[] = {
	{ cmd_help,    "help", "      - prints this message" },
	{ cmd_timeout, "timeout", "   - boot timeout, usage: timeout [<timeout>]" },
	{ cmd_go,      "go!", "       - starts Phoenix-RTOS loaded into memory" },
	{ cmd_cmd,     "cmd", "       - boot command, usage: cmd [<command>]" },
	{ cmd_dump,    "dump", "      - dumps memory, usage: dump <segment>:<offset>" },
	{ cmd_write,   "write", "     - write to memory, usage: write <address> <bytes size> <value>"},
	{ cmd_copy,    "copy", "      - copies data between devices, usage:\n           copy <src device> <src file/LBA> <dst device> <dst file/LBA> [<len>]" },
	{ cmd_memmap,  "mem", "       - prints physical memory map" },
	{ cmd_save,    "save", "      - saves configuration" },
	{ cmd_kernel,  "kernel", "    - loads Phoenix - RTOS, usage: kernel [<boot device>]"},
	{ cmd_app,     "app", "       - loads app, usage:\n           app [<boot device>] [-x] [@]<name>|<name(offset:size)> [imap] [dmap]" },
	{ cmd_map,     "map", "       - define multimap, usage: map <start> <end> <name> <attributes>"},
	{ cmd_syspage, "syspage", "   - shows syspage contents, usage: syspage" },
	{ cmd_phfs,    "phfs", "      - register device in phfs, usage: phfs <alias> <major.minor> [protocol]" },
	{ cmd_devs,    "devs", "      - show registered devs in phfs, usage: devs" },
	{ cmd_console, "console", "   - set console to device, usage: console <major.minor>" },
	{ NULL, NULL, NULL }
};


struct {
	cmd_t cmds[MAX_COMMANDS_NB + 1];
} cmd_common;



/* Auxiliary functions */
static int cmd_cpphfs2phfs(handler_t srcHandler, addr_t srcAddr, size_t srcSz, handler_t dstHandler, addr_t dstAddr, size_t dstSz)
{
	int res, i = 0;
	size_t size, chunk;
	u8 buff[MSG_BUFF_SZ];

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

		lib_printf("\rWriting to address %p", dstAddr);
		cmd_showprogress(i++);
	} while (size > 0 && res > 0);

	return 0;
}


/* Initialization function */

void cmd_init(void)
{
	hal_memcpy(cmd_common.cmds, genericCmds, sizeof(genericCmds));
	hal_appendcmds(cmd_common.cmds);

	script_init();
}


void cmd_default(void)
{
	script_run();
}


/* Function parses commands */

void cmd_parse(char *line)
{
	int k;
	unsigned int p = 0, wp;
	char word[LINESZ + 1], cmd[LINESZ + 1];

	for (;;) {
		if (cmd_getnext(line, &p, "\n", DEFAULT_CITES, word, sizeof(word)) == NULL) {
			lib_printf("\nSyntax error!\n");
			return;
		}
		if (*word == 0)
			 break;

		wp = 0;
		if (cmd_getnext(word, &wp, DEFAULT_BLANKS, DEFAULT_CITES, cmd, sizeof(cmd)) == NULL) {
			lib_printf("\nSyntax error!\n");
			return;
		}

		/* Find command and launch associated function */
		/* TODO: hide cursor while commands are executed */
		for (k = 0; cmd_common.cmds[k].cmd != NULL; k++) {
			if (!hal_strcmp(cmd, cmd_common.cmds[k].cmd)) {
				cmd_common.cmds[k].f(word + wp);
				break;
			}
		}
		if (!cmd_common.cmds[k].cmd)
			lib_printf("\n'%s' - unknown command!\n", cmd);
	}

	return;
}


/* Generic command handlers */

void cmd_help(char *s)
{
	int i, platformCmdsID;

	lib_printf("\n");
	lib_printf("Loader commands:\n");
	lib_printf("----------------\n");

	platformCmdsID = sizeof(genericCmds) / sizeof(cmd_t) - 1;

	for (i = 0; cmd_common.cmds[i].cmd; i++) {
		if (i == platformCmdsID)  {
			lib_printf("\nPlatform specific commands: \n");
			lib_printf("----------------\n");
		}

		lib_printf("%s %s\n", cmd_common.cmds[i].cmd, cmd_common.cmds[i].help);
	}
}


void cmd_timeout(char *s)
{
	char word[LINESZ + 1];
	unsigned int p = 0;

	lib_printf("\n");

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, DEFAULT_CITES, word, sizeof(word)) == NULL) {
		lib_printf("Syntax error!\n");
		return;
	}
	if (*word)
		hal_setLaunchTimeout(lib_strtoul(word, NULL, 16));

	lib_printf("timeout=0x%x\n", hal_getLaunchTimeout());
	return;
}


void cmd_go(char *s)
{
	lib_printf("\n");
	hal_launch();

	return;
}


void cmd_cmd(char *s)
{
	int len;
	char cmd[CMD_SIZE];
	unsigned int p = 0;

	len = min(hal_strlen(s), CMD_SIZE - 1);

	lib_printf("\n");
	cmd_skipblanks(s, &p, DEFAULT_BLANKS);
	s += p;

	if (*s) {
		hal_memcpy(cmd, s, len);
		*((char *)cmd + len) = 0;
	}

	lib_printf("cmd=%s\n", (char *)cmd);

	return;
}


void cmd_dump(char *s)
{
	char word[LINESZ + 1];
	unsigned int p = 0;
	int xsize = 16;
	int ysize = 16;
	unsigned int x, y;
	u32 offs;
	u8 b;

	/* Get address */
	cmd_skipblanks(s, &p, DEFAULT_BLANKS);

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL) {
		lib_printf("\nSize error!\n");
		return;
	}
	if (*word == 0) {
		lib_printf("\nBad segment!\n");
		return;
	}

	offs = lib_strtoul(word, NULL, 16);

	lib_printf("\n");
	lib_printf("Memory dump from %p:\n", offs);
	lib_printf("--------------------------\n");

	for (y = 0; y < ysize; y++) {
		lib_printf("%p   ", offs);

		/* Print byte values */
		for (x = 0; x < xsize; x++) {
			b = *(u8 *)(offs + x);
			if (b & 0xf0)
				lib_printf("%x ", b);
			else
				lib_printf("0%x ", b);
		}
		lib_printf("  ");

		/* Print ASCII representation */
		for (x = 0; x < xsize; x++) {
			b = *(u8 *)(offs + x);
			if ((b <= 32) || (b > 127))
				lib_printf(".", b);
			else
				lib_printf("%c", b);
		}

		lib_printf("\n");

		offs += xsize;
	}

	lib_printf("");
	return;
}



void cmd_write(char *s)
{
	char word[LINESZ + 1];
	unsigned int y, p = 0;
	u32 offs, data, size;

	/* Get address */
	cmd_skipblanks(s, &p, DEFAULT_BLANKS);

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL) {
		lib_printf("\nSize error!\n");
		return;
	}
	if (*word == 0) {
		lib_printf("\nBad segment!\n");
		return;
	}

	offs = lib_strtoul(word, NULL, 16);
	cmd_skipblanks(s, &p, DEFAULT_BLANKS);

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL) {
		lib_printf("\nSize error!\n");
		return;
	}
	if (*word == 0) {
		lib_printf("\nBad size!\n");
		return;
	}

	size = lib_strtoul(word, NULL, 16);

	cmd_skipblanks(s, &p, DEFAULT_BLANKS);

	data = 0x00;

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) != NULL) {
		if (*word != 0)
			data = lib_strtoul(word, NULL, 16);
	}

	for (y = offs; y < offs + size; y++) {
		*(u8 *)(y) = data;
	}

	return;
}


static int cmd_parseDev(handler_t *h, addr_t *offs, size_t *sz, char (*args)[LINESZ + 1], u16 *argsID, u16 argsc)
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


void cmd_copy(char *s)
{
	size_t sz[2];
	addr_t offs[2];
	handler_t h[2];

	u16 argsc = 0, argsID = 0;
	char args[MAX_CMD_ARGS_NB][LINESZ + 1];

	/* Parse all comand's arguments */
	if (cmd_parseArgs(s, args, &argsc) < 0 || argsc < 4) {
		lib_printf("\nWrong arguments!!\n");
		return;
	}

	if (cmd_parseDev(&h[0], &offs[0], &sz[0], args, &argsID, argsc) < 0) {
		lib_printf("\nCannot open file\n");
		return;
	}

	if (cmd_parseDev(&h[1], &offs[1], &sz[1], args, &argsID, argsc) < 0) {
		lib_printf("\nCannot open file\n");
		return;
	}

	/* Copy data between devices */
	if (cmd_cpphfs2phfs(h[0], offs[0], sz[0], h[1], offs[1], sz[1]) < 0)
		lib_printf("\nOperation failed\n");
	else
		lib_printf("\nFinished copying\n");

	phfs_close(h[0]);
	phfs_close(h[1]);

	return;
}


void cmd_memmap(char *s)
{
	/* TODO */
	lib_printf("\nMEMMAP in progress...\n");
}



/* Function saves boot configuration */
void cmd_save(char *s)
{
	/* TODO */
	lib_printf("\nSAVE in progress...\n");
}


void cmd_kernel(char *s)
{
	u8 buff[384];
	void *loffs;
	handler_t handler;
	addr_t minaddr = 0xffffffff, maxaddr = 0;
	addr_t offs = 0;

	u16 argsc = 0;
	char args[MAX_CMD_ARGS_NB][LINESZ + 1];

	Elf32_Ehdr hdr;
	Elf32_Shdr shdr;
	Elf32_Phdr phdr;
	Elf32_Word i = 0, k = 0, size = 0;


	/* Parse arguments */
	if (cmd_parseArgs(s, args, &argsc) < 0) {
		lib_printf("\nWrong arguments!!\n");
		return;
	}

	if (phfs_open(args[0], KERNEL_PATH, 0, &handler) < 0) {
		lib_printf("\nCannot initialize device %s, error: %d\nLook at the device list:", args[0]);
		phfs_showDevs();
		return;
	}

	/* Read ELF header */
	if (phfs_read(handler, offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0) {
		lib_printf("Can't read ELF header!\n");
		return;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		lib_printf("File isn't ELF object!\n");
		return;
	}

	/* Read program segments */
	for (k = 0; k < hdr.e_phnum; k++) {
		offs = hdr.e_phoff + k * sizeof(Elf32_Phdr);
		if (phfs_read(handler, offs, (u8 *)&phdr, (u32)sizeof(Elf32_Phdr)) < 0) {
			lib_printf("Can't read Elf32_Phdr, k=%d!\n", k);
			return;
		}

		if ((phdr.p_type == PHT_LOAD)) {
			/* Calculate kernel memory parameters, omit .bss and .data sections */
			if (phdr.p_flags == (PHF_R + PHF_X)) {
				if (minaddr > phdr.p_paddr)
					minaddr = phdr.p_paddr;
				if (maxaddr < phdr.p_paddr + phdr.p_memsz)
					maxaddr = phdr.p_paddr + phdr.p_memsz;
			}

			loffs = (void *)hal_vm2phym((addr_t)phdr.p_vaddr);

			for (i = 0; i < phdr.p_filesz / sizeof(buff); i++) {
				offs = phdr.p_offset + i * sizeof(buff);
				if (phfs_read(handler, offs, buff, (u32)sizeof(buff)) < 0) {
					lib_printf("\nCan't read segment data, k=%d!\n", k);
					return;
				}
				hal_memcpy(loffs, buff, sizeof(buff));
				loffs += sizeof(buff);

				lib_printf("\rWriting data segment at %p: ", (u32)(loffs));
				cmd_showprogress(i);
			}

			/* Last segment part */
			size = phdr.p_filesz % sizeof(buff);
			if (size != 0) {
				offs = phdr.p_offset + i * sizeof(buff);
				if (phfs_read(handler, offs, buff, size) < 0) {
					lib_printf("\nCan't read last segment data, k=%d!\n", k);
					return;
				}

				hal_memcpy((void *)loffs, buff, size);
			}
		}
	}

	for (k = 0; k < hdr.e_shnum; k++) {
		offs = hdr.e_shoff + k * sizeof(Elf32_Shdr);

		if (phfs_read(handler, offs, (u8 *)&shdr, (u32)sizeof(Elf32_Shdr)) < 0) {
			lib_printf("Can't read Elf32_Shdr, k=%d!\n", k);
			return;
		}

		/* Find .bss section header */
		if (shdr.sh_type == SHT_NOBITS && shdr.sh_flags == (SHF_WRITE | SHF_ALLOC))
			syspage_setKernelBss((void *)shdr.sh_addr, (u32)shdr.sh_size);
	}

	/* TODO: it is temporary solution. It should be defined. */
	syspage_setKernelData(0, 0);

	syspage_setKernelText((void *)minaddr, maxaddr - minaddr);
	hal_setKernelEntry(hdr.e_entry);

	lib_printf("[ok]\n");

	if (phfs_close(handler) < 0)
		lib_printf("\nCannot close file\n");

}


static int cmd_loadApp(handler_t handler, addr_t offs, size_t size, const char *imap, const char *dmap, const char *cmdline, u32 flags)
{
	int res, i = 0;
	Elf32_Ehdr hdr;
	void *start, *end;
	addr_t addr;
	unsigned int attr;
	size_t chunkSz;
	u8 buff[MSG_BUFF_SZ];

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

	/* Copy data to map */
	if (res == dev_isNotMappable) {
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

			lib_printf("\rWriting to address %p ", start);
			cmd_showprogress(i++);

			offs += res;
			size -= res;
		} while (size > 0 && res > 0);

		/* Get map top address after copying */
		syspage_getMapTop(imap, &end);
	}
	else if (res == dev_isMappable) {
		/* User don't provide offset, get from phfs */
		if (offs == 0) {
			if (phfs_getFileAddr(handler, &offs) < 0)
				return ERR_ARG;
		}
		start = (void *)(offs + addr);
		end = start + size;

		lib_printf("\nCode is located in %s map. Data has not been copied.\n", imap);
	}
	else {
		lib_printf("\nDevice returns wrong mapping result.\n");
		return ERR_PHFS_FILE;
	}

	if (syspage_addProg(start, end, imap, dmap, cmdline, flags) < 0) {
		lib_printf("\nCannot add program to syspage\n");
		return ERR_ARG;
	}

	return ERR_NONE;
}


void cmd_app(char *s)
{
	int i, argID = 0;
	u16 cmdArgsc = 0;
	char cmdArgs[MAX_CMD_ARGS_NB][LINESZ + 1];

	char *cmdline;
	unsigned int pos = 0, flags = 0;
	char cmap[8], dmap[8], appName[MAX_APP_NAME_SIZE];
	char appData[3][LINESZ + 1];

	size_t sz = 0;
	addr_t offs = 0;
	handler_t handler;
	phfs_stat_t stat;

	/* Parse command arguments */
	if (cmd_parseArgs(s, cmdArgs, &cmdArgsc) < 0 || cmdArgsc < 2 || cmdArgsc > 6) {
		lib_printf("\nWrong arguments!!\n");
		return;
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
			return;
		}
	}

	if (argID >= cmdArgsc) {
		lib_printf("\nWrong arguments!!\n");
		return;
	}

	/* ARG_2:  Parse application data - cmdline(offset:size) */
	for (i = 0; i < 3; ++i) {
		if (cmd_getnext(cmdArgs[argID], &pos, "@ (:) \t", NULL, appData[i], sizeof(appData[i])) == NULL || *appData[i] == 0)
				break;

		if (i == 0) {
			cmdline = appData[0];
			continue;
		}

		if (lib_ishex(appData[i]) < 0) {
			lib_printf("\nOffset is not a hex value !!\n");
			return;
		}

		if (i == 1)
			offs = lib_strtoul(appData[i], NULL, 16);
		else if (i == 2)
			sz = lib_strtoul(appData[i], NULL, 16);
	}

	/* Get app name from cmdline */
	for (pos = 0; cmdline[pos]; pos++) {
		if (appData[0][pos] == ';')
			break;
	}

	if (pos > MAX_APP_NAME_SIZE) {
		lib_printf("\nApp %s name is too long!\n", cmdline);
		return;
	}

	hal_memcpy(appName, appData[0], pos);
	appName[pos] = '\0';

	/* ARG_3: Get map for instruction section */
	hal_setDefaultIMAP(cmap);
	if ((argID + 1) < cmdArgsc) {
		hal_memcpy(cmap, cmdArgs[++argID], 8);
		cmap[sizeof(cmap) - 1] = '\0';
	}
	/* ARG_4: Get map for data section */
	hal_setDefaultDMAP(dmap);
	if ((argID + 1) < cmdArgsc) {
		hal_memcpy(dmap, cmdArgs[++argID], 8);
		dmap[sizeof(dmap) - 1] = '\0';
	}

	/* If offset and size are set, appName is ommitted and phfs does not use file abstraction */
	if (phfs_open(cmdArgs[0], ((offs == 0 && sz == 0) ? appName : NULL), 0, &handler) < 0) {
		lib_printf("\nWrong arguments!!\n");
		return;
	}

	/* Size is not defined by user */
	if (sz == 0) {
		if (phfs_stat(handler, &stat) < 0) {
			lib_printf("\nCannot get stat from file %s\n", cmdArgs[0]);
			return;
		}
		sz = stat.size;
	}

	lib_printf("\nLoading %s (offs=%p, size=%p, imap=%s, dmap=%s, flags=%x)\n", appName, offs, sz, cmap, dmap, flags);
	cmd_loadApp(handler, offs, sz, cmap, dmap, cmdline, flags);

	if (phfs_close(handler) < 0)
		lib_printf("\nCannot close file\n");
}


void cmd_map(char *s)
{
	u32 attr = 0;
	addr_t start, end;
	u16 argID = 0, argsc = 0;

	u8 namesz;
	char mapname[8];

	unsigned int pos = 0;
	char args[MAX_CMD_ARGS_NB][LINESZ + 1];

	for (argsc = 0; argsc < MAX_CMD_ARGS_NB; ++argsc) {
		if (cmd_getnext(s, &pos, "+ \t", NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	if (argsc < 4) {
		lib_printf("\nWrong arguments!!\n");
		return;
	}

	namesz = (hal_strlen(args[argID]) < 7) ? (hal_strlen(args[argID]) + 1) : 7;
	hal_memcpy(mapname, args[argID], namesz);
	mapname[namesz] = '\0';

	++argID;
	start = lib_strtoul(args[argID], NULL, 16);
	++argID;
	end = lib_strtoul(args[argID], NULL, 16);

	while (++argID < argsc) {
		switch (args[argID][0]) {
		case 'R':
			attr |= mAttrRead;
			break;

		case 'W':
			attr |= mAttrWrite;
			break;

		case 'E':
			attr |= mAttrExec;
			break;

		case 'S':
			attr |= mAttrShareable;
			break;

		case 'C':
			attr |= mAttrCacheable;
			break;

		case 'B':
			attr |= mAttrBufferable;
			break;

		default:
			lib_printf("Wrong attribute - '%c'. Map cannot be created.\n", args[argID][0]);
			return;
		}
	}

	if (syspage_addmap(mapname, (void *)start, (void *)end, attr) < 0) {
		lib_printf("\nMap cannot be created. Check map range and parameters!!\n");
		return;
	}

	lib_printf("\nMap: %s offs: %p size %p has been created.\n", mapname, start, end);

	return;
}


void cmd_syspage(char *s)
{
	syspage_show();
}


void cmd_phfs(char *s)
{
	u16 argsc;
	unsigned int major, minor, pos = 0;
	char args[5][LINESZ + 1];

	for (argsc = 0; argsc < 5; ++argsc) {
		if (cmd_getnext(s, &pos, ". \t", NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	if (argsc < 3 || argsc > 4) {
		lib_printf("\nWrong arguments!!\n");
		return;
	}

	/* Get major/minor */
	major = lib_strtoul(args[1], NULL, 16);
	minor = lib_strtoul(args[2], NULL, 16);

	if (phfs_regDev(args[0], major, minor, (argsc == 3) ? NULL : args[3]) < 0)
		lib_printf("\n%s is not registered!\n", args[0]);
}


void cmd_devs(char *s)
{
	unsigned int pos = 0;

	cmd_skipblanks(s, &pos, DEFAULT_BLANKS);
	s += pos;

	if (*s) {
		lib_printf("\nCommand devs does not take any arguments\n");
		return;
	}

	phfs_showDevs();
}


void cmd_console(char *s)
{
	u16 argsc;
	unsigned int major, minor, pos = 0;
	char args[3][LINESZ + 1];

	for (argsc = 0; argsc < 3; ++argsc) {
		if (cmd_getnext(s, &pos, ". \t", NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	if (argsc != 2) {
		lib_printf("\nWrong number of arguments!!\n");
		return;
	}

	/* Get major/minor */
	major = lib_strtoul(args[0], NULL, 16);
	minor = lib_strtoul(args[1], NULL, 16);
	console_set(major, minor);

	return;
}


/* Auxiliary functions */

/* Function prints progress indicator */
void cmd_showprogress(u32 p)
{
	char *states = "-\\|/";

	lib_printf("%c", states[p % hal_strlen(states)]);
	return;
}


/* Function skips blank characters */
void cmd_skipblanks(char *line, unsigned int *pos, char *blanks)
{
	char c, blfl;
	unsigned int i;

	while ((c = *((char *)(line + *pos))) != 0) {
		blfl = 0;
		for (i = 0; i < hal_strlen(blanks); i++) {
			if (c == *(char *)(blanks + i)) {
				blfl = 1;
				break;
			}
		}
		if (!blfl)
			break;
		(*pos)++;
	}

	return;
}


/* Function retrieves next symbol from line */
char *cmd_getnext(char *line, unsigned int *pos, char *blanks, char *cites, char *word, unsigned int len)
{
	char citefl = 0, c;
	unsigned int i, wp = 0;

	/* Skip leading blank characters */
	cmd_skipblanks(line, pos, blanks);

	wp = 0;
	while ((c = *(char *)(line + *pos)) != 0) {

		/* Test cite characters */
		if (cites) {
			for (i = 0; cites[i]; i++) {
				if (c != cites[i])
					continue;
				citefl ^= 1;
				break;
			}

			/* Go to next iteration if cite character found */
			if (cites[i]) {
				(*pos)++;
				continue;
			}
		}

		/* Test separators */
		for (i = 0; blanks[i]; i++) {
			if (c != blanks[i])
				continue;
			break;
		}
		if (!citefl && blanks[i])
			break;

		word[wp++] = c;
		if (wp == len)
			return NULL;

		(*pos)++;
	}

	if (citefl)
		return NULL;

	word[wp] = 0;

	return word;
}


int cmd_parseArgs(char *s, char (*args)[LINESZ + 1], u16 *argsc)
{
	int i;
	unsigned int pos = 0;

	for (i = 0; *argsc < MAX_CMD_ARGS_NB; ++i) {
		cmd_skipblanks(s, &pos, DEFAULT_BLANKS);
		if (cmd_getnext(s, &pos, DEFAULT_BLANKS, NULL, args[i], sizeof(args[i])) == NULL || *args[i] == 0)
			break;
		(*argsc)++;
	}

	if (!*argsc)
		return ERR_ARG;

	return 0;
}