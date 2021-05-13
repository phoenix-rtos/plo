/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * kernel command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "elf.h"
#include "hal.h"
#include "phfs.h"
#include "syspage.h"


static void cmd_kernelInfo(void)
{
	lib_printf("loads Phoenix - RTOS, usage: kernel [<boot device>]");
}


static int cmd_kernel(char *s)
{
	u8 buff[384];
	void *loffs;
	handler_t handler;
	unsigned int pos = 0;
	addr_t minaddr = 0xffffffff, maxaddr = 0, offs = 0;
	char dev[SIZE_CMD_ARG_LINE + 1];

	Elf32_Ehdr hdr;
	Elf32_Shdr shdr;
	Elf32_Phdr phdr;
	Elf32_Word i = 0, k = 0, size = 0;


	/* Parse arguments */
	cmd_skipblanks(s, &pos, DEFAULT_BLANKS);
	if (cmd_getnext(s, &pos, DEFAULT_BLANKS, NULL, dev, sizeof(dev)) == NULL || dev[0] == 0) {
		lib_printf("Wrong args!\n");
		return ERR_ARG;
	}

	if (phfs_open(dev, KERNEL_PATH, 0, &handler) < 0) {
		lib_printf("\nCannot initialize device %s, error: %d\nLook at the device list:", dev);
		phfs_showDevs();
		return ERR_ARG;
	}

	/* Read ELF header */
	if (phfs_read(handler, offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0) {
		lib_printf("Can't read ELF header!\n");
		return ERR_ARG;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		lib_printf("File isn't ELF object!\n");
		return ERR_ARG;
	}

	/* Read program segments */
	for (k = 0; k < hdr.e_phnum; k++) {
		offs = hdr.e_phoff + k * sizeof(Elf32_Phdr);
		if (phfs_read(handler, offs, (u8 *)&phdr, (u32)sizeof(Elf32_Phdr)) < 0) {
			lib_printf("Can't read Elf32_Phdr, k=%d!\n", k);
			return ERR_ARG;
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
					return ERR_ARG;
				}
				hal_memcpy(loffs, buff, sizeof(buff));
				loffs += sizeof(buff);
			}

			/* Last segment part */
			size = phdr.p_filesz % sizeof(buff);
			if (size != 0) {
				offs = phdr.p_offset + i * sizeof(buff);
				if (phfs_read(handler, offs, buff, size) < 0) {
					lib_printf("\nCan't read last segment data, k=%d!\n", k);
					return ERR_ARG;
				}

				hal_memcpy((void *)loffs, buff, size);
			}
		}
	}

	for (k = 0; k < hdr.e_shnum; k++) {
		offs = hdr.e_shoff + k * sizeof(Elf32_Shdr);

		if (phfs_read(handler, offs, (u8 *)&shdr, (u32)sizeof(Elf32_Shdr)) < 0) {
			lib_printf("Can't read Elf32_Shdr, k=%d!\n", k);
			return ERR_ARG;
		}

		/* Find .bss section header */
		if (shdr.sh_type == SHT_NOBITS && shdr.sh_flags == (SHF_WRITE | SHF_ALLOC))
			syspage_setKernelBss((void *)shdr.sh_addr, (u32)shdr.sh_size);
	}

	/* TODO: it is temporary solution. It should be defined. */
	syspage_setKernelData(0, 0);

	syspage_setKernelText((void *)minaddr, maxaddr - minaddr);
	hal_setKernelEntry(hdr.e_entry);

	if (phfs_close(handler) < 0) {
		lib_printf("\nCannot close file\n");
		return ERR_ARG;
	}

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_kernelReg(void)
{
	const static cmd_t app_cmd = { .name = "kernel", .run = cmd_kernel, .info = cmd_kernelInfo };

	cmd_reg(&app_cmd);
}
