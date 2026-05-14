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


#if defined(__TARGET_RISCV64) || defined(__aarch64__)
#define ELF_WORD Elf64_Word
#define ELF_EHDR Elf64_Ehdr
#define ELF_PHDR Elf64_Phdr
#else
#define ELF_WORD Elf32_Word
#define ELF_EHDR Elf32_Ehdr
#define ELF_PHDR Elf32_Phdr
#endif


static void cmd_kernelInfo(void)
{
	lib_printf("loads Phoenix-RTOS with optional CRC32 verification, usage: kernel [-v <crc32>] <dev> [name]");
}


static int cmd_kernelVerifyCrc(handler_t handler, u32 expectedCrc)
{
	u8 buff[SIZE_MSG_BUFF];
	ssize_t res;
	u32 crc = 0xffffffffU;
	size_t offs = 0;
	phfs_stat_t stat;

	res = phfs_stat(handler, &stat);
	if (res < 0) {
		log_error("\nCan't get file size for CRC verification (%d)", res);
		return (int)res;
	}

	while (offs < stat.size) {
		res = phfs_read(handler, offs, buff, min(sizeof(buff), stat.size - offs));
		if (res <= 0) {
			log_error("\nCan't read data for CRC verification (%d)", res);
			return (int)res;
		}
		crc = lib_crc32(buff, res, crc);
		offs += res;
	}

	crc = crc ^ 0xffffffffU;

	if (crc != expectedCrc) {
		log_error("\nCRC mismatch (expected 0x%08x, got 0x%08x)", expectedCrc, crc);
		return -EIO;
	}

	return EOK;
}


static int cmd_kernel(int argc, char *argv[])
{
	u8 buff[SIZE_MSG_BUFF];
	ssize_t res;
	int opt, posArgc;
	addr_t kernelPAddr = (addr_t)-1;
	u32 expectedCrc = 0;
	int verifyCrc = 0;
	const char *kname;
	handler_t handler;

	size_t elfOffs = 0, segOffs;

	ELF_WORD i;
	ELF_EHDR hdr;
	ELF_PHDR phdr;

	const mapent_t *entry;
	char *endptr;

	/* Parse optional arguments */
	optind = 1;
	for (;;) {
		opt = lib_getopt(argc, argv, "v:");
		if (opt == -1) {
			break;
		}

		switch (opt) {
			case 'v':
				expectedCrc = (u32)lib_strtoul(optarg, &endptr, 0);
				if ((optarg == endptr) || (*endptr != '\0')) {
					log_error("\n%s: Invalid CRC32 value '%s'", argv[0], optarg);
					return CMD_EXIT_FAILURE;
				}
				verifyCrc = 1;
				break;

			default:
				log_error("\n%s: Unknown option", argv[0]);
				return CMD_EXIT_FAILURE;
		}
	}

	/* Parse positional arguments */
	posArgc = argc - optind;
	if ((posArgc < 1) || (posArgc > 2)) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	kname = (posArgc == 2) ? argv[optind + 1] : PATH_KERNEL;

	res = phfs_open(argv[optind], kname, 0, &handler);
	if (res < 0) {
		log_error("\nCannot open %s, on %s (%d)", kname, argv[optind], res);
		return CMD_EXIT_FAILURE;
	}

	/* Verify CRC32 of the raw file on disk before loading */
	if (verifyCrc != 0) {
		res = cmd_kernelVerifyCrc(handler, expectedCrc);
		if (res < 0) {
			phfs_close(handler);
			return CMD_EXIT_FAILURE;
		}
	}

	/* Read ELF header */
	res = phfs_read(handler, elfOffs, &hdr, sizeof(ELF_EHDR));
	if (res < 0) {
		log_error("\nCan't read %s, on %s (%d)", kname, argv[optind], res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		log_error("\n%s isn't an ELF object", kname);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	/* Read program segments */
	for (i = 0; i < hdr.e_phnum; i++) {
		elfOffs = hdr.e_phoff + i * sizeof(ELF_PHDR);
		res = phfs_read(handler, elfOffs, &phdr, sizeof(ELF_PHDR));
		if (res < 0) {
			log_error("\nCan't read %s, on %s (%d)", kname, argv[optind], res);
			phfs_close(handler);
			return CMD_EXIT_FAILURE;
		}

		if (phdr.p_type == (ELF_WORD)PHT_LOAD) {
			entry = syspage_entryAdd(NULL, hal_kernelGetAddress((addr_t)phdr.p_vaddr), phdr.p_memsz, phdr.p_align);
			if (entry == NULL) {
				log_error("\nCannot allocate memory for '%s'", kname);
				phfs_close(handler);
				return CMD_EXIT_FAILURE;
			}

			/* Save kernel's beginning address */
			if ((phdr.p_flags & (ELF_WORD)PHF_X) != 0) {
				kernelPAddr = entry->start;
			}

			elfOffs = phdr.p_offset;

			for (segOffs = 0; segOffs < phdr.p_filesz; elfOffs += res, segOffs += res) {
				res = phfs_read(handler, elfOffs, buff, min(sizeof(buff), phdr.p_filesz - segOffs));
				if (res < 0) {
					log_error("\nCan't read %s, on %s (%d)", kname, argv[optind], res);
					phfs_close(handler);
					return CMD_EXIT_FAILURE;
				}

				hal_memcpy((void *)(entry->start + segOffs), buff, res);
			}
		}
	}

	hal_kernelEntryPoint(hal_kernelGetAddress(hdr.e_entry));
	syspage_kernelPAddrAdd(kernelPAddr);
	phfs_close(handler);

	log_info("\nLoaded %s", kname);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t kernel_cmd __attribute__((section("commands"), used)) = {
	.name = "kernel", .run = cmd_kernel, .info = cmd_kernelInfo
};
