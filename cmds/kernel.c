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


static int cmd_kernel(char *s)
{
	u8 buff[384];
	char *kname;
	void *loffs;
	handler_t handler;
	addr_t minaddr = 0xffffffff, maxaddr = 0, offs = 0;

	Elf32_Ehdr hdr;
	Elf32_Shdr shdr;
	Elf32_Phdr phdr;
	Elf32_Word i = 0, k = 0, size = 0;

	unsigned int argsc;
	cmdarg_t *args;

	/* Parse arguments */
	if ((argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args)) == 0) {
		syspage_showKernel();
		return EOK;
	}
	else if (argsc > 2) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}

	kname = (argsc == 2) ? args[1] : KERNEL_PATH;

	if (phfs_open(args[0], kname, 0, &handler) < 0) {
		log_error("\nCannot open %s, on %s", kname, args[0]);
		return -EINVAL;
	}

	/* Read ELF header */
	if (phfs_read(handler, offs, (u8 *)&hdr, (u32)sizeof(Elf32_Ehdr)) < 0) {
		log_error("\nCan't read %s, on %s", kname, args[0]);
		return -EINVAL;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		log_error("\n%s isn't an ELF object", kname);
		return -EINVAL;
	}

	/* Read program segments */
	for (k = 0; k < hdr.e_phnum; k++) {
		offs = hdr.e_phoff + k * sizeof(Elf32_Phdr);
		if (phfs_read(handler, offs, (u8 *)&phdr, (u32)sizeof(Elf32_Phdr)) < 0) {
			log_error("\nCan't read %s, on %s", kname, args[0]);
			return -EINVAL;
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
					log_error("\nCan't read %s, on %s", kname, args[0]);
					return -EINVAL;
				}
				hal_memcpy(loffs, buff, sizeof(buff));
				loffs += sizeof(buff);
			}

			/* Last segment part */
			size = phdr.p_filesz % sizeof(buff);
			if (size != 0) {
				offs = phdr.p_offset + i * sizeof(buff);
				if (phfs_read(handler, offs, buff, size) < 0) {
					log_error("\nCan't read %s, on %s", kname, args[0]);
					return -EINVAL;
				}

				hal_memcpy((void *)loffs, buff, size);
			}
		}
	}

	/* TODO: find sections based on name */
	for (k = 0; k < hdr.e_shnum; k++) {
		offs = hdr.e_shoff + k * sizeof(Elf32_Shdr);

		if (phfs_read(handler, offs, (u8 *)&shdr, (u32)sizeof(Elf32_Shdr)) < 0) {
			log_error("\nCan't read %s, on %s", kname, args[0]);
			return -EINVAL;
		}

		/* Find .bss section header */
		if (shdr.sh_type == SHT_NOBITS && shdr.sh_flags == (SHF_WRITE | SHF_ALLOC))
			syspage_setKernelBss(hal_vm2phym(shdr.sh_addr), (addr_t)shdr.sh_size);
	}

	/* TODO: it is temporary solution. It should be defined. */
	syspage_setKernelData(0, 0);

	syspage_setKernelText(hal_vm2phym(minaddr), maxaddr - minaddr);
	syspage_setKernelEntry(hal_vm2phym(hdr.e_entry));

	if (phfs_close(handler) < 0) {
		log_error("\nCan't close %s, on %s", kname, args[0]);
		return -EINVAL;
	}

	log_info("\nLoaded %s", kname);

	return EOK;
}


__attribute__((constructor)) static void cmd_kernelReg(void)
{
	const static cmd_t app_cmd = { .name = "kernel", .run = cmd_kernel, .info = cmd_kernelInfo };

	cmd_reg(&app_cmd);
}
