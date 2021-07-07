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
	u8 buff[384];
	const char *kname;
	void *loffs;
	handler_t handler;
	addr_t minaddr = 0xffffffff, maxaddr = 0, offs = 0;

	Elf32_Ehdr hdr;
	Elf32_Shdr shdr;
	Elf32_Phdr phdr;
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
	if ((res = phfs_read(handler, offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr))) < 0) {
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
		if ((res = phfs_read(handler, offs, (u8 *)&phdr, (u32)sizeof(Elf32_Phdr))) < 0) {
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
				if ((res = phfs_read(handler, offs, buff, (u32)sizeof(buff))) < 0) {
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

				hal_memcpy((void *)loffs, buff, size);
			}
		}
	}

	/* TODO: find sections based on name */
	for (k = 0; k < hdr.e_shnum; k++) {
		offs = hdr.e_shoff + k * sizeof(Elf32_Shdr);

		if ((res = phfs_read(handler, offs, (u8 *)&shdr, (u32)sizeof(Elf32_Shdr))) < 0) {
			log_error("\nCan't read %s, on %s", kname, argv[1]);
			return res;
		}

		/* Find .bss section header */
		if (shdr.sh_type == SHT_NOBITS && shdr.sh_flags == (SHF_WRITE | SHF_ALLOC))
			syspage_setKernelBss(hal_kernelGetAddress(shdr.sh_addr), (addr_t)shdr.sh_size);
	}

	/* TODO: it is temporary solution. It should be defined. */
	syspage_setKernelData(0, 0);

	syspage_setKernelText(hal_kernelGetAddress(minaddr), maxaddr - minaddr);
	syspage_setKernelEntry(hal_kernelGetAddress(hdr.e_entry));

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
