/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Verify kernel image ECDSA signature
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz
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

#include <hal/armv8m/stm32/n6/hash.h>
#include <hal/armv8m/stm32/n6/pka.h>

// #if defined(__TARGET_RISCV64) || defined(__aarch64__)
// #define ELF_WORD Elf64_Word
// #define ELF_EHDR Elf64_Ehdr
// #define ELF_PHDR Elf64_Phdr
// #else
#define ELF_WORD Elf32_Word
#define ELF_EHDR Elf32_Ehdr
#define ELF_PHDR Elf32_Phdr
#define ELF_SHDR Elf32_Shdr
// #endif


static void cmd_sigverifInfo(void)
{
	lib_printf("verifies Phoenix-RTOS kernel image signature, usage: sigverif [<dev> [name]]");
}


static int cmd_sigverif(int argc, char *argv[])
{
	u8 buff[SIZE_MSG_BUFF];
	ssize_t res;
	const char *kname;
	handler_t handler;

	size_t elfOffs = 0, segOffs;
	(void)segOffs;

	ELF_WORD i;
	ELF_EHDR hdr;
	ELF_PHDR phdr;
	ELF_SHDR shdr;

	/* Parse arguments */
	if ((argc == 1) || (argc > 3)) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}
	
	kname = (argc == 3) ? argv[2] : PATH_KERNEL;
	
	res = phfs_open(argv[1], kname, 0, &handler);
	if (res < 0) {
		log_error("\nCannot open %s, on %s (%d)", kname, argv[1], res);
		return CMD_EXIT_FAILURE;
	}

	/* Read ELF header */
	res = phfs_read(handler, elfOffs, &hdr, sizeof(ELF_EHDR));
	if (res < 0) {
		log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	if ((hdr.e_ident[0] != 0x7f) || (hdr.e_ident[1] != 'E') || (hdr.e_ident[2] != 'L') || (hdr.e_ident[3] != 'F')) {
		log_error("\n%s isn't an ELF object", kname);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	lib_printf("\n");

	lib_printf("Sections: %d\n", hdr.e_shnum);

	/* Begin feeding data into HASH peripheral */
	res = hash_initDigest(HASH_ALGO_SHA2_384);
	if (res < 0) {
		log_error("\n Cannot begin hashing operation\n");
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}
	hash_feedMessage(&hdr, sizeof(hdr));
	
	/* Todo: parse sections and program segments */

	/* Read program segments */
	for (i = 0; i < hdr.e_phnum; i++) {
		elfOffs = hdr.e_phoff + i * sizeof(ELF_PHDR);
		res = phfs_read(handler, elfOffs, &phdr, sizeof(ELF_PHDR));
		if (res < 0) {
			log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
			phfs_close(handler);
			return CMD_EXIT_FAILURE;
		}

		/* Todo: Feed the hash */
		hash_feedMessage(&phdr, sizeof(ELF_PHDR));
		elfOffs = phdr.p_offset;

		for (segOffs = 0; segOffs < phdr.p_filesz; elfOffs += res, segOffs += res) {
			res = phfs_read(handler, elfOffs, buff, min(sizeof(buff), phdr.p_filesz - segOffs));
			if (res < 0) {
				log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
				phfs_close(handler);
				return CMD_EXIT_FAILURE;
			}
			hash_feedMessage(&phdr, res);
		}
	}

	/* Read sections */
	for (i = 0; i < hdr.e_shnum; i++) {
		elfOffs = hdr.e_shoff + i * sizeof(ELF_SHDR);
		res = phfs_read(handler, elfOffs, &shdr, sizeof(ELF_SHDR));
		if (res < 0) {
			log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
			phfs_close(handler);
			return CMD_EXIT_FAILURE;
		}

		// hash_feedMessage(&shdr, )
	}
	// 	lib_printf("Section %d, size: %d\n", i, shdr.sh_size);

	// 	// if (shdr. == (ELF_WORD)PHT_LOAD) {


	// 	// 	/* Save kernel's beginning address */
		

	// 	// 	elfOffs = phdr.p_offset;

	// 	// 	for (segOffs = 0; segOffs < phdr.p_filesz; elfOffs += res, segOffs += res) {
	// 	// 		res = phfs_read(handler, elfOffs, buff, min(sizeof(buff), phdr.p_filesz - segOffs));
	// 	// 		if (res < 0) {
	// 	// 			log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
	// 	// 			phfs_close(handler);
	// 	// 			return CMD_EXIT_FAILURE;
	// 	// 		}

	// 	// 	}
	// 	// }
	// }


	phfs_close(handler);

	lib_printf("\n");
	log_info("\nVerified %s", kname);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t sigverif_cmd __attribute__((section("commands"), used)) = {
	.name = "sigverif", .run = cmd_sigverif, .info = cmd_sigverifInfo
};
