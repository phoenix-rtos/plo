/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Load kernel and verify it's signature
 *
 * Copyright 2021 2026 Phoenix Systems
 * Author: Hubert Buczynski, Krzysztof Radzewicz
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


#define P256_CURVE        "P-256"
#define HASH_ALGO_STR     "sha2-256"
#define PRIVATE_KEY_BITS  256
#define PRIVATE_KEY_BYTES 32
#define SIGNATURE_SEC     ".signature"

#define ELF_WORD Elf32_Word
#define ELF_EHDR Elf32_Ehdr
#define ELF_PHDR Elf32_Phdr
#define ELF_SHDR Elf32_Shdr


static void cmd_kernelsecureInfo(void)
{
	lib_printf("loads Phoenix-RTOS, usage: kernel-secure <device> <filename> <ecdsa_algorithm> <hash_algorithm> <public_key>");
}


static ssize_t cmd_readString(handler_t h, size_t offs, u8 *out, size_t maxsize)
{
	ssize_t len;
	size_t i = 0;
	char c;
	do {
		if (i >= maxsize) {
			return -ENOMEM;
		}

		len = phfs_read(h, offs, &c, 1);
		if (len < 0) {
			return len;
		}

		offs += len;
		if (c == '\0') {
			out[i] = '\0';

			return i;
		}

		out[i++] = c;
	} while (1);
}


static int cmd_kernelsecure(int argc, char *argv[])
{
	u8 buff[SIZE_MSG_BUFF];
	ssize_t res;
	addr_t kernelPAddr = (addr_t)-1;
	const char *kname;
	handler_t handler;

	size_t elfOffs = 0, segOffs, strtabOffs;

	ELF_WORD i;
	ELF_EHDR hdr;
	ELF_PHDR phdr;
	ELF_SHDR shdr;

	const mapent_t *entry;

	size_t privkeyb64Len = ((PRIVATE_KEY_BYTES + 3 - 1) / 3) * 4; /* Expected lenth of the private key in base64 */

	u8 pubkeyX[PRIVATE_KEY_BYTES];
	u8 pubkeyY[PRIVATE_KEY_BYTES];
	u8 sigR[PRIVATE_KEY_BYTES];
	u8 sigS[PRIVATE_KEY_BYTES];
	u8 hash[PRIVATE_KEY_BYTES];
	int signatureFound = 0;

	/* Parse arguments */
	if (argc != 6) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	kname = argv[2];

	/* Parse ecdsa arguments */

	/* Check curve name validity. Don't return validation errors as there might be more user scripts */
	if (hal_strcmp(argv[3], P256_CURVE) != 0) {
		log_error("\nEliptic curve: %s not supported.", argv[3]);
		return CMD_EXIT_SUCCESS;
	}
	/* Check hash algo name validity */
	if (hal_strcmp(argv[4], HASH_ALGO_STR) != 0) {
		log_error("\nHash algorithm: %s not supported.", argv[4]);
		return CMD_EXIT_SUCCESS;
	}
	if (hal_strlen(argv[5]) < 2 * privkeyb64Len) {
		log_error("\nInvalid public key length, expected: %d", 2 * privkeyb64Len);
		return CMD_EXIT_SUCCESS;
	}
	res = hash_base64_decode(argv[5], privkeyb64Len, pubkeyX, PRIVATE_KEY_BYTES);
	if (res < 0) {
		log_error("\nInvalid base64 format %s", argv[5]);
		return CMD_EXIT_SUCCESS;
	}
	res = hash_base64_decode(argv[5] + privkeyb64Len, privkeyb64Len, pubkeyY, PRIVATE_KEY_BYTES);
	if (res < 0) {
		log_error("\nInvalid base64 format %s", argv[5]);
		return CMD_EXIT_SUCCESS;
	}

	res = phfs_open(argv[1], argv[2], 0, &handler);
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

	(void)hash_initDigest(HASH_ALGO_SHA2_256);
	(void)hash_feedMessage((const u8 *)&hdr, sizeof(ELF_EHDR));

	/* Read program segments */
	for (i = 0; i < hdr.e_phnum; i++) {
		elfOffs = hdr.e_phoff + i * sizeof(ELF_PHDR);
		res = phfs_read(handler, elfOffs, &phdr, sizeof(ELF_PHDR));
		if (res < 0) {
			log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
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
					log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
					phfs_close(handler);
					return CMD_EXIT_FAILURE;
				}
				hal_memcpy((void *)(entry->start + segOffs), buff, res);
				hash_feedMessage(buff, res);
			}
		}
	}

	(void)hash_getDigest(hash, PRIVATE_KEY_BYTES);

	/* Get string table offset */
	elfOffs = hdr.e_shoff + hdr.e_shstrndx * sizeof(ELF_SHDR);
	res = phfs_read(handler, elfOffs, &shdr, sizeof(ELF_SHDR));
	if (res < 0) {
		log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}
	if (shdr.sh_type != SHT_STRTAB) {
		log_error("\n%s: Can't read .string table section", kname);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}
	strtabOffs = shdr.sh_offset;

	/* Read sections to find .signature section */
	for (i = 0; i < hdr.e_shnum; i++) {
		elfOffs = hdr.e_shoff + i * sizeof(ELF_SHDR);
		res = phfs_read(handler, elfOffs, &shdr, sizeof(ELF_SHDR));
		if (res < 0) {
			log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
			phfs_close(handler);
			return CMD_EXIT_FAILURE;
		}

		elfOffs = strtabOffs + shdr.sh_name;
		res = cmd_readString(handler, elfOffs, buff, SIZE_MSG_BUFF);
		if (res < 0) {
			log_error("\n%s: Can't read section name\n", kname);
			phfs_close(handler);
			return CMD_EXIT_FAILURE;
		}

		if (hal_strcmp((const char *)buff, SIGNATURE_SEC) == 0) {
			signatureFound = 1;
			break;
		}
	}
	if (signatureFound == 0) {
		log_error("\n%s: %s section missing", kname, SIGNATURE_SEC);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	elfOffs = shdr.sh_offset;
	res = phfs_read(handler, elfOffs, sigR, PRIVATE_KEY_BYTES);
	if (res < 0) {
		log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}
	res = phfs_read(handler, elfOffs + PRIVATE_KEY_BYTES, sigS, PRIVATE_KEY_BYTES);
	if (res < 0) {
		log_error("\nCan't read %s, on %s (%d)", kname, argv[1], res);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	/* Verify signature */
	res = pka_ecdsaVerify(sigR, sigS, hash, pubkeyX, pubkeyY, PRIVATE_KEY_BITS);
	if (res < 0) {
		log_error("\n%s: failed to verify %s", argv[0], kname);
		phfs_close(handler);
		return CMD_EXIT_FAILURE;
	}

	hal_kernelEntryPoint(hal_kernelGetAddress(hdr.e_entry));
	syspage_kernelPAddrAdd(kernelPAddr);
	phfs_close(handler);

	log_info("\nVerified %s", kname);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t kernel_cmd __attribute__((section("commands"), used)) = {
	.name = "kernel-sec", .run = cmd_kernelsecure, .info = cmd_kernelsecureInfo
};
