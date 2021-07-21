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
#include <lib/lib.h>
#include <phfs/phfs.h>
#include <syspage.h>


static void cmd_kernelInfo(void)
{
	lib_printf("loads Phoenix-RTOS, usage: kernel [<dev> [name]]");
}


static int cmd_kernel(int argc, char *argv[])
{
	u8 buff[SIZE_MSG_BUFF];
	ssize_t res;
	const char *kname;
	handler_t handler;

	size_t elfOffs = 0, segOffs;

	Elf32_Word i;
	Elf32_Ehdr hdr;
	Elf32_Phdr phdr;

	const mapent_t *entry;

	/* Parse arguments */
	if (argc == 1 || argc > 3) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	kname = (argc == 3) ? argv[2] : PATH_KERNEL;

	if ((res = phfs_open(argv[1], kname, 0, &handler)) < 0) {
		log_error("\nCannot open %s, on %s", kname, argv[1]);
		return res;
	}

	/* Read ELF header */
	if ((res = phfs_read(handler, elfOffs, &hdr, sizeof(Elf32_Ehdr))) < 0) {
		log_error("\nCan't read %s, on %s", kname, argv[1]);
		return res;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		log_error("\n%s isn't an ELF object", kname);
		return -EINVAL;
	}

	/* Read program segments */
	for (i = 0; i < hdr.e_phnum; i++) {
		elfOffs = hdr.e_phoff + i * sizeof(Elf32_Phdr);
		if ((res = phfs_read(handler, elfOffs, &phdr, sizeof(Elf32_Phdr))) < 0) {
			log_error("\nCan't read %s, on %s", kname, argv[1]);
			return res;
		}

		if ((phdr.p_type == PHT_LOAD)) {
			if ((entry = syspage_entryAdd(NULL, hal_kernelGetAddress((addr_t)phdr.p_vaddr), phdr.p_memsz, phdr.p_align)) == NULL) {
				log_error("\nCannot allocate memory for '%s'", kname);
				return -ENOMEM;
			}

			elfOffs = phdr.p_offset;

			for (segOffs = 0; segOffs < phdr.p_filesz; elfOffs += res, segOffs += res) {
				if ((res = phfs_read(handler, elfOffs, buff, min(sizeof(buff), phdr.p_filesz - segOffs))) < 0) {
					log_error("\nCan't read %s, on %s", kname, argv[1]);
					return res;
				}

				hal_memcpy((void *)(entry->start + segOffs), buff, res);
			}
		}
	}

	hal_kernelEntryPoint(hal_kernelGetAddress(hdr.e_entry));
	phfs_close(handler);

	log_info("\nLoaded %s", kname);

	return EOK;
}


__attribute__((constructor)) static void cmd_kernelReg(void)
{
	const static cmd_t app_cmd = { .name = "kernel", .run = cmd_kernel, .info = cmd_kernelInfo };

	cmd_reg(&app_cmd);
}
