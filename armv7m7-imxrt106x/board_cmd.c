/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader commands
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski, Marcin Baran
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../errors.h"
#include "../plostd.h"
#include "../phfs.h"
#include "../low.h"
#include "../cmd.h"
#include "../elf.h"

#include "config.h"


/* Arguments position for load comand */
#define MAX_CMD_LOAD_ARGS_NB     6
#define CMD_LOAD_NAME_POS        1
#define CMD_LOAD_OFFSET_POS      2
#define CMD_LOAD_ADDRESS_POS     3
#define CMD_LOAD_SIZE_POS        4
#define CMD_LOAD_DMAP_POS        5


#define CMD_LOAD_MAX_NAME_SIZE   17

#define FLASH_MEM_START_ADDRESS  0x60000000

#define KERNEL_PATH "phoenix"
struct {
	char *name;
	unsigned int pdn;
} devices[] = {
	{ "flash0", PDN_FLASH0 },
	{ "flash1", PDN_FLASH1 },
	{ "com1", PDN_COM1 },
	{ "usb", PDN_USB },
	{ NULL, NULL }
};

u32 kernel_entry = 0;


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


static void cmd_findElfSections(unsigned int pdn, u32 handle, Elf32_Ehdr hdr, u32 kernel_offs)
{
	char name[32];
	Elf32_Shdr shdr;
	Elf32_Word k = 0;
	u32 pos, sh_offset, offs;

	pos = kernel_offs + hdr.e_shoff + hdr.e_shstrndx * sizeof(Elf32_Shdr);
	if (phfs_read(pdn, handle, &pos , (u8 *)&shdr, (u32)sizeof(Elf32_Shdr)) < 0) {
		plostd_printf(ATTR_ERROR, "Can't read Elf32_Shdr, k=%d!\n", k);
		return;
	}

	sh_offset = shdr.sh_offset;

	for (k = 0; k < hdr.e_shnum; k++) {
		offs = kernel_offs + hdr.e_shoff + k * sizeof(Elf32_Shdr);
		if (phfs_read(pdn, handle, &offs , (u8 *)&shdr, (u32)sizeof(Elf32_Shdr)) < 0) {
			plostd_printf(ATTR_ERROR, "Can't read Elf32_Shdr, k=%d!\n", k);
			return;
		}

		pos = sh_offset + shdr.sh_name;
		if (phfs_read(pdn, handle, &pos , (u8 *)name, 32) < 0) {
			plostd_printf(ATTR_ERROR, "Can't read Elf32_Shdr, k=%d!\n", k);
			return;
		}

		/* Check .bss section */
		if (name[0] == '.' && name[1] == 'b' && name[2] == 's' && name[3] == 's') {
			plo_syspage->kernel.bss = (void *)shdr.sh_addr;
			plo_syspage->kernel.bsssz = shdr.sh_size;
		}
		/* Check .data section */
		else if (name[0] == '.' && name[1] == 'd' && name[2] == 'a' && name[3] == 't' && name[4] == 'a') {
			plo_syspage->kernel.data = (void *)shdr.sh_addr;
			plo_syspage->kernel.datasz = shdr.sh_size;
		}
	}
}


void cmd_loadkernel(unsigned int pdn, char *arg, u16 *po)
{
	u32 offs = 0, kernel_offs = 0;
	u32 loffs, handle;
	Elf32_Word i = 0, k = 0, size = 0;
	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;
	u8 buff[384];
	u32 minaddr = 0xffffffff, maxaddr = 0;

	if (pdn < 2)
		kernel_offs = KERNEL_OFFS;

	if ((handle = phfs_open(pdn, arg, 0)) < 0) {
		plostd_printf(ATTR_ERROR, "Cannot initialize %d device\n", pdn);
		return;
	}

	/* Read ELF header */
	offs = kernel_offs;
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
		offs = kernel_offs + hdr.e_phoff + k * sizeof(Elf32_Phdr);
		if (phfs_read(pdn, handle, &offs , (u8 *)&phdr, (u32)sizeof(Elf32_Phdr)) < 0) {
			plostd_printf(ATTR_ERROR, "Can't read Elf32_Phdr, k=%d!\n", k);
			return;
		}

		if ((phdr.p_type == PT_LOAD)) {
			/* Calculate kernel memory parameters, omit .bss and .data sections */
			if (phdr.p_flags == (PF_R + PF_X)) {
				if (minaddr > phdr.p_paddr)
					minaddr = phdr.p_paddr;
				if (maxaddr < phdr.p_paddr + phdr.p_memsz)
					maxaddr = phdr.p_paddr + phdr.p_memsz;
			}

			loffs = phdr.p_vaddr;

			plostd_printf(ATTR_LOADER, "Reading segment %p at %p:  ", phdr.p_vaddr, (loffs));

			for (i = 0; i < phdr.p_filesz / sizeof(buff); i++) {
				offs = kernel_offs + phdr.p_offset + i * sizeof(buff);
				if (phfs_read(pdn, handle, &offs, buff, (u32)sizeof(buff)) < 0) {
					plostd_printf(ATTR_ERROR, "\nCan't read segment data, k=%d!\n", k);
					return;
				}
				low_memcpy((void *)loffs, buff, sizeof(buff));
				loffs += sizeof(buff);
				cmd_showprogress(i);
			}

			/* Last segment part */
			size = phdr.p_filesz % sizeof(buff);
			if (size != 0) {
				offs = phdr.p_offset + i * sizeof(buff);
				if (phfs_read(pdn, handle, &offs, buff, size) < 0) {
					plostd_printf(ATTR_ERROR, "\nCan't read last segment data, k=%d!\n", k);
					return;
				}
			}

			low_memcpy((void *)loffs, buff, size);
			cmd_showprogress(i);
			plostd_printf(ATTR_LOADER, "%c[ok]\n", 8);
		}
	}

	plo_syspage->kernel.data = NULL;
	plo_syspage->kernel.datasz = 0;


	cmd_findElfSections(pdn, handle, hdr, kernel_offs);

	kernel_entry = hdr.e_entry;

	plo_syspage->kernel.text = (void *)minaddr;
	plo_syspage->kernel.textsz = maxaddr - minaddr;
}


void cmd_loadfile(unsigned int pdn, char * arg, u32 offs, u32 addr, u32 size)
{
	int i, handle;
	u8 buff[384];

	if ((handle = phfs_open(pdn, arg, 0)) < 0) {
		plostd_printf(ATTR_ERROR, "Cannot initialize source!\n");
		return;
	}

	plostd_printf(ATTR_LOADER, "Reading program at %p", addr);

	for (i = 0; i < size / sizeof(buff); i++) {
		if (phfs_read(pdn, handle, &offs, buff, (u32)sizeof(buff)) < 0) {
			plostd_printf(ATTR_ERROR, "\nCan't read segment data, i=%d!\n", i);
			return;
		}

		low_memcpy((void *)addr, buff, sizeof(buff));
		addr += sizeof(buff);
		cmd_showprogress(i);
	}

	/* Last segment part */
	size = size % sizeof(buff);
	if (size != 0) {
		if (phfs_read(pdn, handle, &offs, buff, size) < 0) {
			plostd_printf(ATTR_ERROR, "\nCan't read last segment data, i=%d!\n", i);
			return;
		}
	}

	low_memcpy((void *)addr, buff, size);
	cmd_showprogress(i);
	plostd_printf(ATTR_LOADER, "%c[ok]\n", 8);
}


static void cmd_syspageUpdate(char *name, u32 addr, u32 size, u32 map)
{
	unsigned int pos = 0;

	plo_syspage->progs[plo_syspage->progssz].start = addr;
	plo_syspage->progs[plo_syspage->progssz].end = addr + size;
	plo_syspage->progs[plo_syspage->progssz].dmap = map;

	*plo_syspage->arg++ = 'X';
	low_memcpy((void *)plo_syspage->arg, name, plostd_strlen(name));

	plo_syspage->arg += plostd_strlen(name);
	*plo_syspage->arg++ = ' ';

	low_memcpy(&plo_syspage->progs[plo_syspage->progssz].cmdline, name, plostd_strlen(name));

	for (pos = plostd_strlen(name); pos < (17 - 1); ++pos)
		plo_syspage->progs[plo_syspage->progssz].cmdline[pos] = 0;

	plo_syspage->progssz++;
}


static int cmd_checkDev(const char *dev)
{
	unsigned int dn;

	/* Show boot devices if parameter is empty */
	if (*dev == 0) {
		plostd_printf(ATTR_LOADER, "\nBoot devices: ");

		for (dn = 0; devices[dn].name; dn++)
			plostd_printf(ATTR_LOADER, "%s ", devices[dn].name);
		plostd_printf(ATTR_LOADER, "\n");

		return ERR_ARG;
	}

	for (dn = 0; devices[dn].name; dn++)  {
		if (!plostd_strcmp(dev, devices[dn].name))
			break;
	}

	if (!devices[dn].name) {
		plostd_printf(ATTR_ERROR, "\n'%s' - unknown boot device!\n", dev);
		return ERR_ARG;
	}

	return dn;
}


void cmd_load(char *args)
{
	int dn, i;
	u16 po = 0, argsc = 0;   /* TODO: variable 'po' should be used in cmd_loadkernel function */
	unsigned int pos = 0;

	u32 offs, addr, size, map;
	char name[CMD_LOAD_MAX_NAME_SIZE];
	char word[MAX_CMD_LOAD_ARGS_NB][LINESZ + 1];

	/* Parse arguments */
	for (i = 0; argsc < MAX_CMD_LOAD_ARGS_NB; ++i) {
		cmd_skipblanks(args, &pos, DEFAULT_BLANKS);
		if (cmd_getnext(args, &pos, DEFAULT_BLANKS, NULL, word[i], sizeof(word[i])) == NULL || *word[i] == 0)
			break;

		argsc++;
	}

	if (!argsc) {
		plostd_printf(ATTR_ERROR, "\nChoose appropriate device!\n");
		return;
	}

	if ((dn = cmd_checkDev(word[0])) < 0)
		return;

	/* Load file based on provided arguments */
	switch (argsc) {
		/* Loading kernel from default directory */
		case 1:
			plostd_printf(ATTR_LOADER, "\nLoading kernel\n");
			cmd_loadkernel(devices[dn].pdn, "phoenix-armv7m7-imxrt106x.elf", &po);
			return;

		/* Loading kernel from provided directory */
		case 2:
			size = sizeof(word[CMD_LOAD_NAME_POS]) < (CMD_LOAD_MAX_NAME_SIZE - 1) ? sizeof(word[CMD_LOAD_NAME_POS]) : (CMD_LOAD_MAX_NAME_SIZE - 1);
			low_memcpy(name, word[CMD_LOAD_NAME_POS], size);

			plostd_printf(ATTR_LOADER, "\nLoading kernel from %s\n", name);
			cmd_loadkernel(devices[dn].pdn, name, &po);
			name[size] = '\0';
			return;

		/* Loading program */
		case 6:
			size = sizeof(word[CMD_LOAD_NAME_POS]) < (CMD_LOAD_MAX_NAME_SIZE - 1) ? sizeof(word[CMD_LOAD_NAME_POS]) : (CMD_LOAD_MAX_NAME_SIZE - 1);
			low_memcpy(name, word[CMD_LOAD_NAME_POS], size);
			name[size] = '\0';

			offs = plostd_ahtoi(word[CMD_LOAD_OFFSET_POS]);
			addr = plostd_ahtoi(word[CMD_LOAD_ADDRESS_POS]);
			size = plostd_ahtoi(word[CMD_LOAD_SIZE_POS]);
			map = plostd_ahtoi(word[CMD_LOAD_DMAP_POS]);

			plostd_printf(ATTR_LOADER, "\nLoading program %s (offs=%p, addr=%p, size=%p)\n", name, offs, addr, size);

			/* In case of loading data from flash memory, only syspage needs to be updated. */
			if (addr < FLASH_MEM_START_ADDRESS)
				cmd_loadfile(devices[dn].pdn, name, offs, addr, size);

			cmd_syspageUpdate(name, addr, size, map);
			return;

		default:
			plostd_printf(ATTR_ERROR, "\nWrong number of arguments.\n");
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


void cmd_lspci(char *s)
{
	plostd_printf(ATTR_LOADER, "\nlspci is not supported on this board\n");
}
