/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Loader commands
 *
 * Copyright 2012, 2017, 2020 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Pawel Kolodziej, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "errors.h"
#include "low.h"
#include "plostd.h"
#include "phfs.h"
#include "elf.h"
#include "cmd.h"
#include "syspage.h"
#include "script.h"


const cmd_t genericCmds[] = {
	{ cmd_help,    "help", "    - prints this message" },
	{ cmd_timeout, "timeout", " - boot timeout, usage: timeout [<timeout>]" },
	{ cmd_go,      "go!", "     - starts Phoenix-RTOS loaded into memory" },
	{ cmd_cmd,     "cmd", "     - boot command, usage: cmd [<command>]" },
	{ cmd_dump,    "dump", "    - dumps memory, usage: dump <segment>:<offset>" },
	{ cmd_write,   "write", "   - write to memory, usage: write <address> <bytes size> <value>"},
	{ cmd_copy,    "copy", "    - copies data between devices, usage:\n           copy <src device> <src file/LBA> <dst device> <dst file/LBA> [<len>]" },
	{ cmd_memmap,  "mem", "     - prints physical memory map" },
	{ cmd_save,    "save", "    - saves configuration" },
	{ cmd_kernel,  "kernel", "  - loads Phoenix - RTOS, usage: kernel [<boot device>]"},
	{ cmd_app,     "app", "     - loads app, usage: load [<boot device>] <name>(<offset:size>) [dmap] [imap]" },
	{ cmd_map,     "map", "     - define multimap, usage: map <start> <end> <name> <attributes>"},
	{ cmd_syspage, "syspage", " - shows syspage contents, usage: syspage" },
	{ NULL, NULL, NULL }
};


struct {
	cmd_t cmds[MAX_COMMANDS_NB + 1];
	cmd_device_t *devs;
} cmd_common;



/* Auxiliary functions */

static int cmd_checkDev(const char *dev)
{
	unsigned int dn;

	/* Show boot devices if parameter is empty */
	if (*dev == 0) {
		plostd_printf(ATTR_LOADER, "\nBoot devices: ");

		for (dn = 0; cmd_common.devs[dn].name; dn++)
			plostd_printf(ATTR_LOADER, "%s ", cmd_common.devs[dn].name);
		plostd_printf(ATTR_LOADER, "\n");

		return ERR_ARG;
	}

	for (dn = 0; cmd_common.devs[dn].name; dn++)  {
		if (!plostd_strcmp(dev, cmd_common.devs[dn].name))
			break;
	}

	if (!cmd_common.devs[dn].name) {
		plostd_printf(ATTR_ERROR, "\n'%s' - unknown boot device!\n", dev);
		return ERR_ARG;
	}

	return dn;
}


static int cmd_parseArgs(char *s, char (*args)[LINESZ + 1], u16 *argsc, int *dn)
{
	int i;
	unsigned int pos = 0;

	for (i = 0; *argsc < MAX_CMD_LOAD_ARGS_NB; ++i) {
		cmd_skipblanks(s, &pos, DEFAULT_BLANKS);
		if (cmd_getnext(s, &pos, DEFAULT_BLANKS, NULL, args[i], sizeof(args[i])) == NULL || *args[i] == 0)
			break;
		(*argsc)++;
	}

	if (!*argsc)
		return -1;

	if ((*dn = cmd_checkDev(args[0])) < 0)
		return -1;

	return 0;
}




/* Initialization function */

void cmd_init(void)
{
	low_initdevs(&cmd_common.devs);

	low_memcpy(cmd_common.cmds, genericCmds, sizeof(genericCmds));
	low_appendcmds(cmd_common.cmds);

	script_init();
}


void cmd_default(void)
{
	script_run();
}



/* Function parses loader commands */
void cmd_parse(char *line)
{
	int k;
	char word[LINESZ + 1], cmd[LINESZ + 1];
	unsigned int p = 0, wp;

	for (;;) {
		if (cmd_getnext(line, &p, ";", DEFAULT_CITES, word, sizeof(word)) == NULL) {
			plostd_printf(ATTR_ERROR, "\nSyntax error!\n");
			return;
		}
		if (*word == 0)
			 break;

		wp = 0;
		if (cmd_getnext(word, &wp, DEFAULT_BLANKS, DEFAULT_CITES, cmd, sizeof(cmd)) == NULL) {
			plostd_printf(ATTR_ERROR, "\nSyntax error!\n");
			return;
		}

		/* Find command and launch associated function */
		for (k = 0; cmd_common.cmds[k].cmd != NULL; k++) {

			if (!plostd_strcmp(cmd, cmd_common.cmds[k].cmd)) {
				cmd_common.cmds[k].f(word + wp);
				break;
			}
		}
		if (!cmd_common.cmds[k].cmd)
			plostd_printf(ATTR_ERROR, "\n'%s' - unknown command!\n", cmd);
	}

	return;
}



/* Generic command handlers */

void cmd_help(char *s)
{
	int k, e;

	plostd_printf(ATTR_LOADER, "\n");
	plostd_printf(ATTR_LOADER, "Loader commands:\n");
	plostd_printf(ATTR_LOADER, "----------------\n");

	e = sizeof(genericCmds) / sizeof(cmd_t) - 1;

	for (k = 0; cmd_common.cmds[k].cmd; k++) {
		if (k == e)  {
			plostd_printf(ATTR_LOADER, "\nPlatform specific commands: \n");
			plostd_printf(ATTR_LOADER, "----------------\n");
		}

		plostd_printf(ATTR_LOADER, "%s %s\n", cmd_common.cmds[k].cmd, cmd_common.cmds[k].help);
	}
}


void cmd_timeout(char *s)
{
	char word[LINESZ + 1];
	unsigned int p = 0;

	plostd_printf(ATTR_LOADER, "\n");

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, DEFAULT_CITES, word, sizeof(word)) == NULL) {
		plostd_printf(ATTR_ERROR, "Syntax error!\n");
		return;
	}
	if (*word)
		low_setLaunchTimeout(plostd_ahtoi(word));

	plostd_printf(ATTR_LOADER, "timeout=0x%x\n", low_getLaunchTimeout());
	return;
}


void cmd_go(char *s)
{
	plostd_printf(ATTR_INIT, "\n\n");
	low_launch();

	return;
}


void cmd_cmd(char *s)
{
	int l;
	char cmd[CMD_SIZE];
	unsigned int p = 0;

	l = min(plostd_strlen(s), CMD_SIZE - 1);

	plostd_printf(ATTR_LOADER, "\n");
	cmd_skipblanks(s, &p, DEFAULT_BLANKS);
	s += p;

	if (*s) {
		low_memcpy(cmd, s, l);
		*((char *)cmd + l) = 0;
	}

	plostd_printf(ATTR_LOADER, "cmd=%s\n", (char *)cmd);

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
		plostd_printf(ATTR_ERROR, "\nSize error!\n");
		return;
	}
	if (*word == 0) {
		plostd_printf(ATTR_ERROR, "\nBad segment!\n");
		return;
	}

	offs = plostd_ahtoi(word);

	plostd_printf(ATTR_LOADER, "\n");
	plostd_printf(ATTR_NONE, "Memory dump from %p:\n", offs);
	plostd_printf(ATTR_NONE, "--------------------------\n");

	for (y = 0; y < ysize; y++) {
		plostd_printf(ATTR_NONE, "%p   ", offs);

		/* Print byte values */
		for (x = 0; x < xsize; x++) {
			b = *(u8 *)(offs + x);
			if (b & 0xf0)
				plostd_printf(ATTR_NONE, "%x ", b);
			else
				plostd_printf(ATTR_NONE, "0%x ", b);
		}
		plostd_printf(ATTR_NONE, "  ");

		/* Print ASCII representation */
		for (x = 0; x < xsize; x++) {
			b = *(u8 *)(offs + x);
			if ((b <= 32) || (b > 127))
				plostd_printf(ATTR_NONE, ".", b);
			else
				plostd_printf(ATTR_NONE, "%c", b);
		}

		plostd_printf(ATTR_NONE, "\n");

		offs += xsize;
	}

	plostd_printf(ATTR_LOADER, "");
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
		plostd_printf(ATTR_ERROR, "\nSize error!\n");
		return;
	}
	if (*word == 0) {
		plostd_printf(ATTR_ERROR, "\nBad segment!\n");
		return;
	}

	offs = plostd_ahtoi(word);
	cmd_skipblanks(s, &p, DEFAULT_BLANKS);

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL) {
		plostd_printf(ATTR_ERROR, "\nSize error!\n");
		return;
	}
	if (*word == 0) {
		plostd_printf(ATTR_ERROR, "\nBad size!\n");
		return;
	}

	size = plostd_ahtoi(word);

	cmd_skipblanks(s, &p, DEFAULT_BLANKS);

	data = 0x00;

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) != NULL) {
		if (*word != 0)
			data = plostd_ahtoi(word);
	}

	for (y = offs; y < offs + size; y++) {
		*(u8 *)(y) = data;
	}

	return;
}


void cmd_copy(char *s)
{
	/* TODO */
	plostd_printf(ATTR_LOADER, "\nCOPY in progress...\n");
}


void cmd_memmap(char *s)
{
	/* TODO */
	plostd_printf(ATTR_LOADER, "\nMEMMAP in progress...\n");
}



/* Function saves boot configuration */
void cmd_save(char *s)
{
	/* TODO */
	plostd_printf(ATTR_LOADER, "\nSAVE in progress...\n");
}


void cmd_kernel(char *s)
{
	Elf32_Shdr shdr;
	u8 buff[384];
	handle_t handle;
	addr_t minaddr = 0xffffffff, maxaddr = 0;
	addr_t offs = 0, kernelOffs = 0;
	void *loffs;

	int pdn;
	u16 argsc = 0;
	char args[MAX_CMD_LOAD_ARGS_NB][LINESZ + 1];

	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;
	Elf32_Word i = 0, k = 0, size = 0;


	/* Parse arguments */
	if (cmd_parseArgs(s, args, &argsc, &pdn) < 0) {
		plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
		return;
	}

	plostd_printf(ATTR_LOADER, "\nOpening device %s ...\n", args[0]);
	handle = phfs_open(pdn, "phoenix-armv7m7-imxrt106x.elf", 0);

	if (handle.h < 0) {
		plostd_printf(ATTR_ERROR, "\nCannot initialize device: %s\n", args[0]);
		return;
	}

	if (handle.offs > 0)
		kernelOffs = DISK_KERNEL_OFFS;

	/* Read ELF header */
	offs = kernelOffs;
	if (phfs_read(pdn, handle, &offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0) {
		plostd_printf(ATTR_ERROR, "Can't read ELF header!\n");
		return;
	}

	if ((hdr.e_ident[0] != 0x7f) && (hdr.e_ident[1] != 'E') && (hdr.e_ident[2] != 'L') && (hdr.e_ident[3] != 'F')) {
		plostd_printf(ATTR_ERROR, "File isn't ELF object!\n");
		return;
	}

	/* Read program segments */
	for (k = 0; k < hdr.e_phnum; k++) {
		offs = kernelOffs + hdr.e_phoff + k * sizeof(Elf32_Phdr);
		if (phfs_read(pdn, handle, &offs , (u8 *)&phdr, (u32)sizeof(Elf32_Phdr)) < 0) {
			plostd_printf(ATTR_ERROR, "Can't read Elf32_Phdr, k=%d!\n", k);
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

			loffs = (void *)phdr.p_vaddr;
			for (i = 0; i < phdr.p_filesz / sizeof(buff); i++) {
				offs = kernelOffs + phdr.p_offset + i * sizeof(buff);
				if (phfs_read(pdn, handle, &offs, buff, (u32)sizeof(buff)) < 0) {
					plostd_printf(ATTR_ERROR, "\nCan't read segment data, k=%d!\n", k);
					return;
				}
				low_memcpy(loffs, buff, sizeof(buff));
				loffs += sizeof(buff);

				plostd_printf(ATTR_LOADER, "\rWriting data segment at %p: ", (u32)(loffs));
				cmd_showprogress(i);
			}

			/* Last segment part */
			size = phdr.p_filesz % sizeof(buff);
			if (size != 0) {
				offs = kernelOffs + phdr.p_offset + i * sizeof(buff);
				if (phfs_read(pdn, handle, &offs, buff, size) < 0) {
					plostd_printf(ATTR_ERROR, "\nCan't read last segment data, k=%d!\n", k);
					return;
				}

				low_memcpy((void *)loffs, buff, size);
			}
		}
	}

	for (k = 0; k < hdr.e_shnum; k++) {
		offs = kernelOffs + hdr.e_shoff + k * sizeof(Elf32_Shdr);

		if (phfs_read(pdn, handle, &offs , (u8 *)&shdr, (u32)sizeof(Elf32_Shdr)) < 0) {
			plostd_printf(ATTR_ERROR, "Can't read Elf32_Shdr, k=%d!\n", k);
			return;
		}

		/* Find .bss section header */
		if (shdr.sh_type == SHT_NOBITS && shdr.sh_flags == (SHF_WRITE | SHF_ALLOC))
			syspage_setKernelBss((void *)shdr.sh_addr, (u32)shdr.sh_size);
	}

	syspage_setKernelData(0, 0);

	syspage_setKernelText((void *)minaddr, maxaddr - minaddr);
	low_setKernelEntry(hdr.e_entry);

	plostd_printf(ATTR_LOADER, "%c[ok]\n", 8);
}



static int cmd_loadApp(unsigned int pdn, const char *name, addr_t offs, u32 size, const char *imap, const char *dmap)
{
	int i;
	handle_t handle;
	u8 buff[384];
	Elf32_Ehdr hdr;

	u32 res;
	void *start, *end, *currAddr;

	handle = phfs_open(pdn, name, 0);
	if (handle.h < 0) {
		plostd_printf(ATTR_ERROR, "\nCannot initialize source: %s!\n", name);
		return -1;
	}

	/* Check ELF header */
	if (phfs_read(pdn, handle, &offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0) {
		plostd_printf(ATTR_ERROR, "\nCan't read ELF header!\n");
		return -1;
	}

	if ((hdr.e_ident[0] != 0x7f) && (hdr.e_ident[1] != 'E') && (hdr.e_ident[2] != 'L') && (hdr.e_ident[3] != 'F')) {
		plostd_printf(ATTR_ERROR, "\nFile isn't ELF object!\n");
		return -1;
	}

	/* Get file start address */
	if (syspage_getMapTop(imap, &start) < 0) {
		plostd_printf(ATTR_ERROR, "\n%s does not exist!\n", imap);
		return -1;
	}


	currAddr = start;
	/* Get data from memory storage */
	if (size) {
		/* Check wheter device is in the the same map as imap */
		if ((void *)handle.offs <= (start + offs) && (void *)handle.offs >= start) {
			start += offs;
			end = start + size;
			plostd_printf(ATTR_LOADER, "Code is located in %s map. Data has not been copied.\n", imap);
		}
		/* Get data from memory and copy it to specific imap */
		else {
			for (i = 0; i < size / sizeof(buff); i++) {
				if (phfs_read(pdn, handle, &offs, buff, (u32)sizeof(buff)) < 0) {
					plostd_printf(ATTR_ERROR, "\nCan't read segment data, i=%d!\n", i);
					return -1;
				}

				if (syspage_write2Map(imap, buff, sizeof(buff)) < 0)
					return -1;

				currAddr += sizeof(buff);
				offs += sizeof(buff);
				plostd_printf(ATTR_LOADER, "\rReading adress %p / %p: ", currAddr, start + size);
				cmd_showprogress(i);
			}

			/* Last segment part */
			res = size % sizeof(buff);
			if (res != 0) {
				if (phfs_read(pdn, handle, &offs, buff, res) < 0) {
					plostd_printf(ATTR_ERROR, "\nCan't read last segment data, i=%d!\n", i);
					return -1;
				}

				if (syspage_write2Map(imap, buff, res) < 0)
					return -1;
			}

			currAddr += res;
			plostd_printf(ATTR_LOADER, "\rReading adress %p / %p: ", currAddr, start + size);

			/* Get file end address */
			if (syspage_getMapTop(imap, &end) < 0) {
				plostd_printf(ATTR_ERROR, "\n%s does not exist!\n", imap);
				return -1;
			}
		}
	}
	/* Get data from external source i.e. serial or USB device */
	else {
		if (syspage_write2Map(imap, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0)
			return -1;

		i = 0;
		do {
			if ((res = phfs_read(pdn, handle, &offs, buff, (u32)sizeof(buff))) < 0) {
				plostd_printf(ATTR_ERROR, "\nCan't read segment data, i=%d!\n", i);
				return -1;
			}

			if (syspage_write2Map(imap, buff, res) < 0)
				return -1;

			currAddr += res;
			plostd_printf(ATTR_LOADER, "\rWriting to adress %p ", currAddr);
			cmd_showprogress(i++);

		} while (res > 0);

		/* Get file end address */
		if (syspage_getMapTop(imap, &end) < 0) {
			plostd_printf(ATTR_ERROR, "\n%s does not exist!\n", imap);
			return -1;
		}
	}

	syspage_addProg(start, end, imap, dmap, name);

	plostd_printf(ATTR_LOADER, "%c[ok]\n", 8);

	return 0;
}



void cmd_app(char *s)
{
	int i = 0, pdn, argID = 0;
	unsigned int pos = 0;
	char cmdArgs[MAX_CMD_LOAD_ARGS_NB][LINESZ + 1];
	u16 cmdArgsc = 0;

	u32 size = 0;
	addr_t offs = 0;
	u16 namesz;
	char *name;
	char appArgs[3][LINESZ + 1];
	char cmap[8], dmap[8];


	/* Parse command arguments */
	if (cmd_parseArgs(s, cmdArgs, &cmdArgsc, &pdn) < 0) {
		plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
		return;
	}

	/* Check app name and aliases */
	++argID;
	name = cmdArgs[argID];
	if (name[0] == '@') {
		if (script_expandAlias(&name) < 0) {
			plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
			return;
		}
	}

	/* Parse program name, offset and size */
	for (i = 0; i < 3; ++i) {
		cmd_skipblanks(name, &pos, "( : )\t");
		if (cmd_getnext(name, &pos, "( : )\t", NULL, appArgs[i], sizeof(appArgs[i])) == NULL || *appArgs[i] == 0)
			break;

		if (i == 0 ) {
			namesz = (plostd_strlen(appArgs[i]) < (MAX_APP_NAME_SIZE - 1)) ? (plostd_strlen(appArgs[i]) + 1) : (MAX_APP_NAME_SIZE - 1);
			appArgs[0][namesz] = '\0';
		}
		else if (i == 1) {
			offs = plostd_ahtoi(appArgs[i]);
		}
		else if (i == 2) {
			size = plostd_ahtoi(appArgs[i]);
		}
	}

	/* Set default program parameters */
	low_setDefaultIMAP(cmap);
	low_setDefaultDMAP(dmap);

	/* Get map for code section */
	if ((argID + 1) < cmdArgsc)
		low_memcpy(cmap, cmdArgs[++argID], 8);

	/* Get map for data section */
	if ((argID + 1) < cmdArgsc)
		low_memcpy(dmap, cmdArgs[++argID], 8);


	if (offs != 0 && size != 0)
		plostd_printf(ATTR_LOADER, "\nLoading %s (offs=%p, size=%p, cmap=%s, dmap=%s)\n", appArgs[0], offs, size, cmap, dmap);
	else
		plostd_printf(ATTR_LOADER, "\nLoading %s (offs=UNDEF, size=UNDEF, imap=%s, dmap=%s)\n", appArgs[0], cmap, dmap);

	cmd_loadApp(cmd_common.devs[pdn].pdn, appArgs[0], offs, size, cmap, dmap);

	return;
}


void cmd_map(char *s)
{
	int i;
	u16 argID = 0, argsc = 0;
	addr_t start, end;
	u32 attr = 0;

	u8 namesz;
	char mapname[8];

	unsigned int pos = 0;
	char args[MAX_CMD_LOAD_ARGS_NB][LINESZ + 1];

	for (i = 0; argsc < MAX_CMD_LOAD_ARGS_NB; ++i) {
		cmd_skipblanks(s, &pos, "+ \t");
		if (cmd_getnext(s, &pos, "+ \t", NULL, args[i], sizeof(args[i])) == NULL || *args[i] == 0)
			break;
		argsc++;
	}

	if (argsc < 4) {
		plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
		return;
	}

	namesz = (plostd_strlen(args[argID]) < 7) ? (plostd_strlen(args[argID]) + 1) : 7;
	low_memcpy(mapname, args[argID], namesz);
	mapname[namesz] = '\0';

	++argID;
	start = plostd_ahtoi(args[argID]);
	++argID;
	end = plostd_ahtoi(args[argID]);

	while (++argID < argsc) {
		switch (args[argID][0]) {
		case 'R':
			attr |= mAttrRead;
			break;

		case 'W':
			attr |= mAttrWrite;
			break;

		case 'E':
			attr |= maAttrExec;
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
			plostd_printf(ATTR_ERROR, "Wrong attribute - '%c'. Map cannot be created.\n", args[argID][0]);
			return;
		}
	}

	if (syspage_addmap(mapname, (void *)start, (void *)end, attr) < 0) {
		plostd_printf(ATTR_ERROR, "\nMap cannot be created. Check map range and parameters!!\n");
		return;
	}

	plostd_printf(ATTR_LOADER, "\nMap: %s offs: %p size %p has been created.\n", mapname, start, end);

	return;
}


void cmd_syspage(char *s)
{
	syspage_show();
}



/* Auxiliary functions */

/* Function prints progress indicator */
void cmd_showprogress(u32 p)
{
	char *states = "-\\|/";

	plostd_printf(ATTR_LOADER, "%c", states[p % plostd_strlen(states)]);
	return;
}


/* Function skips blank characters */
void cmd_skipblanks(char *line, unsigned int *pos, char *blanks)
{
	char c, blfl;
	unsigned int i;

	while ((c = *((char *)(line + *pos))) != 0) {
		blfl = 0;
		for (i = 0; i < plostd_strlen(blanks); i++) {
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
