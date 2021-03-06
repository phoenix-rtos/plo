/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Load kernel
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

#include <hal/hal.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_kernelInfo(void)
{
	lib_printf("loads Phoenix-RTOS, usage: kernel [<dev> [name]]");
}


static int cmd_kernel(int argc, char *argv[])
{
	ssize_t res;
	u8 buff[384], strtab[128];
	const char *kname;
	void *loffs;
	handler_t handler;
	addr_t minaddr = 0xffffffff, maxaddr = 0, offs = 0;

	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;
	Elf32_Shdr shstrtab;
	Elf32_Shdr shdr;
	Elf32_Word i = 0, k = 0, size = 0;

	/* Parse arguments */
	if (argc == 1) {
		syspage_showKernel();
		return EOK;
	}
	else if (argc > 3) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	kname = (argc == 3) ? argv[2] : PATH_KERNEL;

	if ((res = phfs_open(argv[1], kname, 0, &handler)) < 0) {
		log_error("\nCannot open %s, on %s", kname, argv[1]);
		return res;
	}

	/* Read ELF header */
	if ((res = phfs_read(handler, offs, &hdr, sizeof(Elf32_Ehdr))) < 0) {
		log_error("\nCan't read %s, on %s", kname, argv[1]);
		return res;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		log_error("\n%s isn't an ELF object", kname);
		return -EINVAL;
	}

	/* Read program segments */
	for (k = 0; k < hdr.e_phnum; k++) {
		offs = hdr.e_phoff + k * sizeof(Elf32_Phdr);
		if ((res = phfs_read(handler, offs, &phdr, sizeof(Elf32_Phdr))) < 0) {
			log_error("\nCan't read %s, on %s", kname, argv[1]);
			return res;
		}

		if ((phdr.p_type == PHT_LOAD)) {
			/* Calculate kernel memory parameters, omit .bss and .data sections */
			if (phdr.p_flags == (PHF_R + PHF_X)) {
				if (minaddr > phdr.p_paddr)
					minaddr = phdr.p_paddr;
				if (maxaddr < phdr.p_paddr + phdr.p_memsz)
					maxaddr = phdr.p_paddr + phdr.p_memsz;
			}

			loffs = (void *)hal_kernelGetAddress((addr_t)phdr.p_vaddr);

			for (i = 0; i < phdr.p_filesz / sizeof(buff); i++) {
				offs = phdr.p_offset + i * sizeof(buff);
				if ((res = phfs_read(handler, offs, buff, sizeof(buff))) < 0) {
					log_error("\nCan't read %s, on %s", kname, argv[1]);
					return res;
				}
				hal_memcpy(loffs, buff, sizeof(buff));
				loffs += sizeof(buff);
			}

			/* Last segment part */
			size = phdr.p_filesz % sizeof(buff);
			if (size != 0) {
				offs = phdr.p_offset + i * sizeof(buff);
				if ((res = phfs_read(handler, offs, buff, size)) < 0) {
					log_error("\nCan't read %s, on %s", kname, argv[1]);
					return res;
				}

				hal_memcpy(loffs, buff, size);
			}
		}
	}

	/* Read string table section header */
	offs = hdr.e_shoff + hdr.e_shstrndx * sizeof(Elf32_Shdr);
	if ((res = phfs_read(handler, offs, &shstrtab, sizeof(Elf32_Shdr))) < 0) {
		log_error("\nCan't read %s, on %s", kname, argv[1]);
		return res;
	}

	if (shstrtab.sh_size > sizeof(strtab)) {
		log_error("\nCan't read %s, on %s", kname, argv[1]);
		return -ENOMEM;
	}

	/* Read string table */
	offs = shstrtab.sh_offset;
	if ((res = phfs_read(handler, offs, strtab, shstrtab.sh_size)) < 0) {
		log_error("\nCan't read %s, on %s", kname, argv[1]);
		return res;
	}

	/* Set syspage kernel entry and .text section */
	syspage_setKernelEntry(hal_kernelGetAddress(hdr.e_entry));
	syspage_setKernelText(hal_kernelGetAddress(minaddr), maxaddr - minaddr);

	/* Set syspage kernel .data and .bss sections */
	for (k = 0; k < hdr.e_shnum; k++) {
		offs = hdr.e_shoff + k * sizeof(Elf32_Shdr);

		if ((res = phfs_read(handler, offs, &shdr, sizeof(Elf32_Shdr))) < 0) {
			log_error("\nCan't read %s, on %s", kname, argv[1]);
			return res;
		}

		if (!hal_strcmp((const char *)strtab + shdr.sh_name, ".data"))
			syspage_setKernelData(hal_kernelGetAddress(shdr.sh_addr), shdr.sh_size);
		else if (!hal_strcmp((const char *)strtab + shdr.sh_name, ".bss"))
			syspage_setKernelBss(hal_kernelGetAddress(shdr.sh_addr), shdr.sh_size);
	}

	if ((res = phfs_close(handler)) < 0) {
		log_error("\nCan't close %s, on %s", kname, argv[1]);
		return res;
	}

	log_info("\nLoaded %s", kname);

	return EOK;
}


__attribute__((constructor)) static void cmd_kernelReg(void)
{
	const static cmd_t app_cmd = { .name = "kernel", .run = cmd_kernel, .info = cmd_kernelInfo };

	cmd_reg(&app_cmd);
}
