/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader commands
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
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


struct {
	char *name;
	unsigned int pdn;
} devices[] = {
	{ "flash0", PDN_FLASH0 },
	{ "flash1", PDN_FLASH1 },
	{ "com1", PDN_COM1 },
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

void cmd_loadkernel(unsigned int pdn, char *arg, u16 *po)
{
	u32 offs;
	u32 loffs;
	Elf32_Word i = 0, k = 0, size = 0;
	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;
	u8 buff[384];
	u32 minaddr = 0xffffffff, maxaddr = 0;

	if (phfs_open(pdn, NULL, 0) < 0) {
		plostd_printf(ATTR_ERROR, "Cannot initialize flash memory!\n");
		return;
	}

	/* Read ELF header */
	/* TODO: get kernel adress from partition table */
	offs = KERNEL_OFFS;
	if (phfs_read(pdn, 0, &offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0) {
		plostd_printf(ATTR_ERROR, "Can't read ELF header!\n");
		return;
	}

	if ((hdr.e_ident[0] != 0x7f) && (hdr.e_ident[1] != 'E') && (hdr.e_ident[2] != 'L') && (hdr.e_ident[3] != 'F')) {
		plostd_printf(ATTR_ERROR, "File isn't ELF object!\n");
		return;
	}

	/* Read program segments */
	for (k = 0; k < hdr.e_phnum; k++) {
		offs = KERNEL_OFFS + hdr.e_phoff + k * sizeof(Elf32_Phdr);
		if (phfs_read(pdn, 0, &offs , (u8 *)&phdr, (u32)sizeof(Elf32_Phdr)) < 0) {
			plostd_printf(ATTR_ERROR, "Can't read Elf32_Phdr, k=%d!\n", k);
			return;
		}

		if ((phdr.p_type == PT_LOAD)) {
			/* Calculate kernel memory parameters */
			if (minaddr > phdr.p_paddr)
				minaddr = phdr.p_paddr;
			if (maxaddr < phdr.p_paddr + phdr.p_memsz)
				maxaddr = phdr.p_paddr + phdr.p_memsz;

			loffs = phdr.p_vaddr;

			plostd_printf(ATTR_LOADER, "Reading segment %p at %p:  ",
				phdr.p_vaddr, (loffs));

			for (i = 0; i < phdr.p_filesz / sizeof(buff); i++) {
				offs = KERNEL_OFFS + phdr.p_offset + i * sizeof(buff);
				if (phfs_read(pdn, 0, &offs, buff, (u32)sizeof(buff)) < 0) {
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
				offs = KERNEL_OFFS + phdr.p_offset + i * sizeof(buff);
				if (phfs_read(pdn, 0, &offs, buff, size) < 0) {
					plostd_printf(ATTR_ERROR, "\nCan't read last segment data, k=%d!\n", k);
					return;
				}
			}

			low_memcpy((void *)loffs, buff, size);
			cmd_showprogress(i);
			plostd_printf(ATTR_LOADER, "%c[ok]\n", 8);
		}
	}

	kernel_entry = hdr.e_entry;
	plo_syspage.progssz = 0;
	plo_syspage.pbegin = minaddr;
	plo_syspage.pend = maxaddr;
}

void cmd_loadfile(unsigned int pdn, u32 offs, u32 addr, u32 size)
{
	int i;
	u8 buff[384];

	if (phfs_open(pdn, NULL, 0) < 0) {
		plostd_printf(ATTR_ERROR, "Cannot initialize flash memory!\n");
		return;
	}

	plostd_printf(ATTR_LOADER, "Reading program at %p", addr);

	for (i = 0; i < size / sizeof(buff); i++) {
		if (phfs_read(pdn, 0, &offs, buff, (u32)sizeof(buff)) < 0) {
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
		if (phfs_read(pdn, 0, &offs, buff, size) < 0) {
			plostd_printf(ATTR_ERROR, "\nCan't read last segment data, i=%d!\n", i);
			return;
		}
	}

	low_memcpy((void *)addr, buff, size);
	cmd_showprogress(i);
	plostd_printf(ATTR_LOADER, "%c[ok]\n", 8);
}


void cmd_load(char *s)
{
	char word[LINESZ + 1];
	unsigned int p = 0, dn;
	u16 po = 0;
	u32 offs, addr, size;
	char name[17];

	cmd_skipblanks(s, &p, DEFAULT_BLANKS);
	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL) {
		plostd_printf(ATTR_ERROR, "\nSize error!\n");
		return;
	}

	/* Show boot devices if parameter is empty */
	if (*word == 0) {
		plostd_printf(ATTR_LOADER, "\nBoot devices: ");
		for (dn = 0; devices[dn].name; dn++)
			plostd_printf(ATTR_LOADER, "%s ", devices[dn].name);
		plostd_printf(ATTR_LOADER, "\n");
		return;
	}

	for (dn = 0; devices[dn].name; dn++)  {
		if (!plostd_strcmp(word, devices[dn].name))
			break;
	}

	if (!devices[dn].name) {
		plostd_printf(ATTR_ERROR, "\n'%s' - unknown boot device!\n", word);
		return;
	}

	cmd_skipblanks(s, &p, DEFAULT_BLANKS);

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL ||
			*word == 0 || !plostd_strcmp(word, "kernel")) {
		/* Load default kernel if no other args or name is kernel*/
		plostd_printf(ATTR_LOADER, "\nLoading kernel\n");
		cmd_loadkernel(devices[dn].pdn, NULL, &po);
		return;
	}
	size = plostd_strlen(word) < 16 ? plostd_strlen(word) : 16;
	low_memcpy(name, word, size);
	name[size] = '\0';

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL ||
			*word == 0) {
		plostd_printf(ATTR_ERROR, "\nOffs error!\n");
		return;
	}

	offs = plostd_ahtoi(word);

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL ||
			*word == 0) {
		plostd_printf(ATTR_ERROR, "\nAddress error!\n");
		return;
	}

	addr = plostd_ahtoi(word);

	if (cmd_getnext(s, &p, DEFAULT_BLANKS, NULL, word, sizeof(word)) == NULL ||
			*word == 0) {
		plostd_printf(ATTR_ERROR, "\nSize error!\n");
		return;
	}

	size = plostd_ahtoi(word);

	plostd_printf(ATTR_LOADER, "\nLoading program %s (offs=%p, addr=%p, size=%p)\n",
		name, offs, addr, size);

	cmd_loadfile(devices[dn].pdn, offs, addr, size);

	plo_syspage.progs[plo_syspage.progssz].start = addr;
	plo_syspage.progs[plo_syspage.progssz].end = addr + size;
	plo_syspage.progs[plo_syspage.progssz++].mapno = 0;

	*syspage_arg_ptr++ = 'X';
	low_memcpy((void *)syspage_arg_ptr, name, plostd_strlen(name));
	syspage_arg_ptr += plostd_strlen(name);
	*syspage_arg_ptr++ = ' ';
	low_memcpy(&plo_syspage.progs[0].cmdline, name, plostd_strlen(name));

	for (p = plostd_strlen(name); p < 16; ++p)
		plo_syspage.progs[0].cmdline[p] = 0;

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
