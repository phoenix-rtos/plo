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

#define MSG_BUFF_SZ 0x100


typedef struct {
	unsigned int dn;
	handle_t handle;

	addr_t dataOffs;
	u32 datasz;
} phfs_conf_t;


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

static int cmd_checkDev(const char *devName, unsigned int *dn)
{
	unsigned int i;

	/* Show boot devices if parameter is empty */

	if (*devName == 0) {
		plostd_printf(ATTR_LOADER, "\nBoot devices: ");

		for (i = 0; cmd_common.devs[i].name; i++)
			plostd_printf(ATTR_LOADER, "%s ", cmd_common.devs[i].name);
		plostd_printf(ATTR_LOADER, "\n");

		return ERR_ARG;
	}

	for (i = 0; cmd_common.devs[i].name; i++)  {
		if (!plostd_strcmp(devName, cmd_common.devs[i].name))
			break;
	}

	if (!cmd_common.devs[i].name) {
		plostd_printf(ATTR_ERROR, "\n'%s' - unknown boot device!\n", devName);
		return ERR_ARG;
	}

	*dn = i;

	return 0;
}


static int cmd_parseArgs(char *s, char (*args)[LINESZ + 1], u16 *argsc)
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


static int cmd_cpphfs2phfs(phfs_conf_t *src, phfs_conf_t *dst)
{
	int res, i = 0;
	u32 size, chunk;
	u8 buff[MSG_BUFF_SZ];

	if (src->handle.offs != 0) {
		if (src->datasz == 0 && src->datasz == 0) {
			size = 0;
		}
		else {
			if (src->datasz == 0 || dst->datasz == 0)
				size = (src->datasz > dst->datasz) ? src->datasz : dst->datasz;
			else
				size = (src->datasz < dst->datasz) ? src->datasz : dst->datasz;
		}
	}
	else {
		size = -1;
	}

	do {
		if (size > sizeof(buff))
			chunk = sizeof(buff);
		else
			chunk = size;

		if ((res = phfs_read(src->dn, src->handle, &src->dataOffs, buff, chunk)) < 0) {
			plostd_printf(ATTR_ERROR, "\nCan't read segment data\n");
			return ERR_PHFS_FILE;
		}

		if (phfs_write(dst->dn, dst->handle, &dst->dataOffs, buff, res, 1) < 0) {
			plostd_printf(ATTR_ERROR, "\nCan't write segment data!\n");
			return ERR_PHFS_FILE;
		}

		if (src->handle.offs != 0) {
			src->dataOffs += res;
			size -= res;
		}

		dst->dataOffs += res;
		plostd_printf(ATTR_LOADER, "\rWriting to adress %p", dst->dataOffs + (addr_t)dst->handle.offs);
		cmd_showprogress(i++);
	} while (size > 0 && res > 0);

	return 0;
}


static int cmd_cpphfs2map(phfs_conf_t *src, const char *map)
{
	void *addr;
	int res, i = 0;
	u32 size, chunk;
	u8 buff[MSG_BUFF_SZ];

	if (syspage_getMapTop(map, &addr) < 0) {
		plostd_printf(ATTR_ERROR, "\n%s does not exist!\n", map);
		return ERR_ARG;
	}

	size = (src->handle.offs != 0) ? src->datasz : -1;

	do {
		if (size > sizeof(buff))
			chunk = sizeof(buff);
		else
			chunk = size;

		if ((res = phfs_read(src->dn, src->handle, &src->dataOffs, buff, chunk)) < 0) {
			plostd_printf(ATTR_ERROR, "\nCan't read segment data\n");
			return ERR_PHFS_FILE;
		}

		if (syspage_write2Map(map, buff, res) < 0)
			return ERR_ARG;

		if (src->handle.offs != 0) {
			src->dataOffs += res;
			size -= res;
		}

		addr += res;
		plostd_printf(ATTR_LOADER, "\rWriting to adress %p ", addr);
		cmd_showprogress(i++);
	} while (size > 0 && res > 0);

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



/* Function parses commands */
void cmd_parse(char *line)
{
	int k;
	unsigned int p = 0, wp;
	char word[LINESZ + 1], cmd[LINESZ + 1];

	for (;;) {
		if (cmd_getnext(line, &p, "\n", DEFAULT_CITES, word, sizeof(word)) == NULL) {
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
		/* TODO: hide cursor while commands are executed */
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
	int i, platformCmdsID;

	plostd_printf(ATTR_LOADER, "\n");
	plostd_printf(ATTR_LOADER, "Loader commands:\n");
	plostd_printf(ATTR_LOADER, "----------------\n");

	platformCmdsID = sizeof(genericCmds) / sizeof(cmd_t) - 1;

	for (i = 0; cmd_common.cmds[i].cmd; i++) {
		if (i == platformCmdsID)  {
			plostd_printf(ATTR_LOADER, "\nPlatform specific commands: \n");
			plostd_printf(ATTR_LOADER, "----------------\n");
		}

		plostd_printf(ATTR_LOADER, "%s %s\n", cmd_common.cmds[i].cmd, cmd_common.cmds[i].help);
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
	plostd_printf(ATTR_NONE, "\n");
	low_launch();

	return;
}


void cmd_cmd(char *s)
{
	int len;
	char cmd[CMD_SIZE];
	unsigned int p = 0;

	len = min(plostd_strlen(s), CMD_SIZE - 1);

	plostd_printf(ATTR_LOADER, "\n");
	cmd_skipblanks(s, &p, DEFAULT_BLANKS);
	s += p;

	if (*s) {
		low_memcpy(cmd, s, len);
		*((char *)cmd + len) = 0;
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

/* Function parse device name or parameters. There is distinction whether it is storage or external dev.
 * For external dev, name shall be provided. For storage device offset and size shall be provided. */
static int cmd_parseDev(phfs_conf_t *dev, const char (*args)[LINESZ + 1], u16 *argsID, u16 cmdArgsc)
{
	if (plostd_ishex(args[++(*argsID)]) < 0) {
		plostd_printf(ATTR_LOADER, "\nOpening device %s ..\n", cmd_common.devs[dev->dn].name);
		dev->handle = phfs_open(dev->dn, args[(*argsID)], 0);

		if (dev->handle.h < 0) {
			plostd_printf(ATTR_ERROR, "Cannot initialize source: %s!\n", cmd_common.devs[dev->dn].name);
			return ERR_PHFS_IO;
		}
	}
	else {
		/* check whether size is provided */
		if (((*argsID) + 1) >= cmdArgsc || plostd_ishex(args[(*argsID) + 1]) < 0)
			return -1;

		dev->dataOffs = plostd_ahtoi(args[(*argsID)]);

		dev->datasz = plostd_ahtoi(args[++(*argsID)]);

		plostd_printf(ATTR_LOADER, "\nOpening device %s ..\n", cmd_common.devs[dev->dn].name);
		dev->handle = phfs_open(dev->dn, NULL, 0);
		if (dev->handle.h < 0) {
			plostd_printf(ATTR_ERROR, "Cannot initialize source: %s!\n", cmd_common.devs[dev->dn].name);
			return ERR_PHFS_IO;
		}
	}

	return 0;
}


void cmd_copy(char *s)
{
	phfs_conf_t phfses[2];

	u16 cmdArgsc = 0, argsID = 0;
	char cmdArgs[MAX_CMD_ARGS_NB][LINESZ + 1];

	phfses[0].datasz = phfses[1].datasz = 0;
	phfses[0].dataOffs = phfses[1].dataOffs = 0;


	/* Parse all comand's arguments */
	if (cmd_parseArgs(s, cmdArgs, &cmdArgsc) < 0 || cmdArgsc < 4) {
		plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
		return;
	}

	/* Parse source parameters */
	if (cmd_checkDev(cmdArgs[argsID], &phfses[0].dn) < 0)
		return;

	if (cmd_parseDev(&phfses[0], cmdArgs, &argsID, cmdArgsc) < 0) {
		phfs_close(phfses[0].dn, phfses[0].handle);
		return;
	}

	/* Parse destination parameters */
	if (cmd_checkDev(cmdArgs[++argsID], &phfses[1].dn) < 0)
		return;

	if (cmd_parseDev(&phfses[1], cmdArgs, &argsID, cmdArgsc) < 0) {
		phfs_close(phfses[0].dn, phfses[0].handle);
		phfs_close(phfses[1].dn, phfses[1].handle);
		return;
	}

	/* Copy data between devices */
	cmd_cpphfs2phfs(&phfses[0], &phfses[1]);

	plostd_printf(ATTR_LOADER, "\nFinished copying\n");

	phfs_close(phfses[0].dn, phfses[0].handle);
	phfs_close(phfses[1].dn, phfses[1].handle);

	return;
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

	unsigned int pdn;
	u16 argsc = 0;
	char args[MAX_CMD_ARGS_NB][LINESZ + 1];

	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;
	Elf32_Word i = 0, k = 0, size = 0;


	/* Parse arguments */
	if (cmd_parseArgs(s, args, &argsc) < 0) {
		plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
		return;
	}

	if (cmd_checkDev(args[0], &pdn) < 0)
		return;

	plostd_printf(ATTR_LOADER, "\nOpening device %s ...\n", args[0]);
	handle = phfs_open(pdn, KERNEL_PATH, 0);

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

	/* TODO: it is temporary solution. It should be defined. */
	syspage_setKernelData(0, 0);

	syspage_setKernelText((void *)minaddr, maxaddr - minaddr);
	low_setKernelEntry(hdr.e_entry);

	plostd_printf(ATTR_LOADER, "%c[ok]\n", 8);
}


static int cmd_loadApp(phfs_conf_t *phfs, const char *name, const char *imap, const char *dmap)
{
	Elf32_Ehdr hdr;
	void *start, *end;

	phfs->handle = phfs_open(phfs->dn, name, 0);
	if (phfs->handle.h < 0) {
		plostd_printf(ATTR_ERROR, "\nCannot initialize source: %s!\n", name);
		return ERR_PHFS_IO;
	}


	/* Check ELF header */
	if (phfs_read(phfs->dn, phfs->handle, &phfs->dataOffs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0) {
		plostd_printf(ATTR_ERROR, "\nCan't read ELF header!\n");
		return ERR_PHFS_FILE;
	}

	if ((hdr.e_ident[0] != 0x7f) && (hdr.e_ident[1] != 'E') && (hdr.e_ident[2] != 'L') && (hdr.e_ident[3] != 'F')) {
		plostd_printf(ATTR_ERROR, "\nFile isn't ELF object!\n");
		return ERR_PHFS_FILE;
	}

	/* Align map top, so the app begin is aligned */
	if (syspage_alignMapTop(imap) < 0)
		return ERR_ARG;

	/* Get instruction begining address */
	if (syspage_getMapTop(imap, &start) < 0) {
		plostd_printf(ATTR_ERROR, "\n%s does not exist!\n", imap);
		return ERR_ARG;
	}

	/* Get data from memory storage */
	if (phfs->datasz) {
		/* Check wheter device is in the the same map as imap */
		if ((void *)phfs->handle.offs <= (start + phfs->dataOffs) && (void *)phfs->handle.offs >= start) {
			start += phfs->dataOffs;
			end = start + phfs->datasz;
			plostd_printf(ATTR_LOADER, "Code is located in %s map. Data has not been copied.\n", imap);
		}
		else {
			cmd_cpphfs2map(phfs, imap);

			/* Get file end address */
			if (syspage_getMapTop(imap, &end) < 0) {
				plostd_printf(ATTR_ERROR, "\n%s does not exist!\n", imap);
				return ERR_ARG;
			}
		}
	}
	/* Get data from external source i.e. serial or USB device */
	else {
		if (syspage_write2Map(imap, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0)
			return ERR_ARG;

		cmd_cpphfs2map(phfs, imap);

		/* Get file end address */
		if (syspage_getMapTop(imap, &end) < 0) {
			plostd_printf(ATTR_ERROR, "\n%s does not exist!\n", imap);
			return ERR_ARG;
		}
	}

	syspage_addProg(start, end, imap, dmap, name);
	plostd_printf(ATTR_LOADER, "%c[ok]\n", 8);

	return 0;
}


void cmd_app(char *s)
{
	u16 cmdArgsc = 0;
	int i = 0, argID = 0;
	char cmdArgs[MAX_CMD_ARGS_NB][LINESZ + 1];

	u16 namesz;
	char *name;
	unsigned int pos = 0;
	char cmap[8], dmap[8];
	char appData[3][LINESZ + 1];

	phfs_conf_t phfs = {.datasz = 0, .dataOffs = 0};

	/* Parse command arguments */
	if (cmd_parseArgs(s, cmdArgs, &cmdArgsc) < 0) {
		plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
		return;
	}

	if (cmd_checkDev(cmdArgs[0], &phfs.dn) < 0)
		return;

	/* Check app name and aliases */
	++argID;
	name = cmdArgs[argID];

	if (name[0] == '@') {
		if (script_expandAlias(&name) < 0) {
			plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
			return;
		}
	}

	/* Parse program name with args, offset and size */
	for (i = 0; i < 3; ++i) {
		cmd_skipblanks(name, &pos, "@ ( : )\t");
		if (cmd_getnext(name, &pos, "( : )\t", NULL, appData[i], sizeof(appData[i])) == NULL || *appData[i] == 0)
			break;

		if (i == 0 ) {
			namesz = (plostd_strlen(appData[i]) < (MAX_APP_NAME_SIZE - 1)) ? (plostd_strlen(appData[i]) + 1) : (MAX_APP_NAME_SIZE - 1);
			appData[0][namesz] = '\0';
		}
		else if (i == 1) {
			if (plostd_ishex(appData[i]) < 0) {
				plostd_printf(ATTR_ERROR, "\nOffset is not a hex value !!\n");
				return;
			}
			phfs.dataOffs = plostd_ahtoi(appData[i]);
		}
		else if (i == 2) {
			if (plostd_ishex(appData[i]) < 0) {
				plostd_printf(ATTR_ERROR, "\nSize is not a hex value !!\n");
				return;
			}

			phfs.datasz = plostd_ahtoi(appData[i]);
		}
	}

	/* Set default maps */
	low_setDefaultIMAP(cmap);
	low_setDefaultDMAP(dmap);

	/* Get map for code section */
	if ((argID + 1) < cmdArgsc) {
		low_memcpy(cmap, cmdArgs[++argID], 8);
		cmap[sizeof(cmap) - 1] = '\0';
	}
	/* Get map for data section */
	if ((argID + 1) < cmdArgsc) {
		low_memcpy(dmap, cmdArgs[++argID], 8);
		dmap[sizeof(dmap) - 1] = '\0';
	}


	if (phfs.dataOffs != 0 && phfs.datasz != 0)
		plostd_printf(ATTR_LOADER, "\nLoading %s (offs=%p, size=%p, cmap=%s, dmap=%s)\n", appData[0], phfs.dataOffs, phfs.datasz, cmap, dmap);
	else
		plostd_printf(ATTR_LOADER, "\nLoading %s (offs=UNDEF, size=UNDEF, imap=%s, dmap=%s)\n", appData[0], cmap, dmap);

	cmd_loadApp(&phfs, appData[0], cmap, dmap);

	return;
}


void cmd_map(char *s)
{
	int i;
	u32 attr = 0;
	addr_t start, end;
	u16 argID = 0, argsc = 0;

	u8 namesz;
	char mapname[8];

	unsigned int pos = 0;
	char args[MAX_CMD_ARGS_NB][LINESZ + 1];

	for (i = 0; argsc < MAX_CMD_ARGS_NB; ++i) {
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
