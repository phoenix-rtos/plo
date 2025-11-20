/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * encrypt external memory on STM32N6
 *
 * Copyright 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>

#include <devices/flash-stm32xspi/xspi_common.h>
#include <hal/armv8m/stm32/n6/otp.h>


#define MEMCRYPT_ENCR             "-encr"
#define MEMCRYPT_ENCR_LEN         5
#define MEMCRYPT_KEY              "-key"
#define MEMCRYPT_KEY_LEN          4
#define MEMCRPYT_AES256           "aes256:"
#define MEMCRYPT_AES256_LEN       7
#define MEMCRPYT_AES128           "aes128:"
#define MEMCRPYT_AES128_LEN       7
#define MEMCRPYT_NOEKEON          "noekeon:"
#define MEMCRPYT_NOEKEON_LEN      8
#define MEMCRYPT_KEY_EXPLICIT     "x:"
#define MEMCRYPT_KEY_EXPLICIT_LEN 2
#define MEMCRYPT_KEY_RANDOM       "r"
#define MEMCRYPT_KEY_RANDOM_LEN   1
#define MEMCRYPT_KEY_OTP          "p:"
#define MEMCRYPT_KEY_OTP_LEN      2

#define MEMCYPT_MAX_KEYSIZE 256


static void cmd_memcryptInfo(void)
{
	lib_printf("Configure external memory encryption. Use <-h> to see usage");
}


static void cmd_memcryptUsage(const char *name)
{
	lib_printf(
			"Usage: %s d saddr eaddr -encr algo:mode -key [r|p|x]:[addr:key]\n"
			"where:\n"
			"\td - device controller number (major.minor) \n"
			"\tsaddr - start memory device address to be encrypted\n"
			"\teaddr - end memory device address to be encrypted (inclusive)\n"
			"\talgo - used cipher: aes128, aes256, noekeon\n"
			"\tmode - used mode of operation: 1, 2, ...\n"
			"\tr - generate random key\n"
			"\tp - use key stored in OTP memory. Requires addr - first OTP word containing the key\n"
			"\tx - provide key explicitly. Requires key - value in hexadecimal\n",
			name);
}


static int cmd_memcryptGetUserKey(const char *string, size_t maxSize, u8 *out)
{
	size_t size;
	u32 t, i;
	const char *p;
	if ((string[0] == '0') && (string[1] == 'x')) {
		string += 2;
	}
	size = hal_strlen(string);
	if ((size <= 0) || ((size * 2) > maxSize)) {
		return -EINVAL;
	}
	p = string + size - 1;
	i = 0;
	while (p >= string) {
		if (!lib_isdigit(*p) && !lib_isalpha(*p)) {
			return -EINVAL;
		}
		t = *p - '0';
		if (t > 9) {
			t = (*p | 0x20) - 'a' + 10;
		}
		if (t >= 16) {
			return -EINVAL;
		}

		if (i % 2 == 0) {
			out[i / 2] = 0;
		}
		out[i / 2] += t << ((i % 2) * 4);

		p--;
		i++;
	}

	return EOK;
}


static int cmd_memcryptGetOtpKey(int fuse, u8 *out, size_t keysize)
{
	int ret;
	u32 val = 0;
	size_t i = 0;
	if ((keysize % 4) != 0) {
		return -EINVAL;
	}
	while (i < keysize) {
		ret = otp_read(fuse, &val);
		if (ret < 0) {
			return ret;
		}
		*((u32 *)out) = val;
		i += 4;
	}
	return EOK;
}


static int cmd_memcrypt(int argc, char *argv[])
{
	int ret, major, minor, fuse;
	addr_t start, end;
	u32 cipher, mode, keysize;
	static u8 keyBuffer[MEMCYPT_MAX_KEYSIZE] = {};
	xspi_memcrypt_args_t args = {};

	lib_printf("\n");

	/* Memcrypt only checks if device number is correct and parses string arguments.
	 * Argument validity is checked by the device. */

	if (argc != 8) {
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	major = lib_strtoul(argv[1], &argv[1], 0);
	if (*argv[1] != '.') {
		log_error("\nInvalid device.\n");
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}
	minor = lib_strtoul(argv[1] + 1, &argv[1], 0);
	if (*argv[1] != '\0') {
		log_error("\nInvalid device.\n");
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	ret = devs_check(major, minor);
	if (ret < 0) {
		log_error("\nInvalid device: (%d,%d)\n", major, minor);
		return -EINVAL;
	}
	/* Add check if device is DEV_ENCR_STORAGE */
	if (major != DEV_CRYP_STORAGE) {
		log_error("\nInvalid device class: %d\n", major);
		return -EINVAL;
	}

	/* start end */
	start = lib_strtoul(argv[2], &argv[2], 0);
	if (*argv[2] != '\0') {
		log_error("\nInvalid start address.\n");
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}
	end = lib_strtoul(argv[3], &argv[3], 0);
	if (*argv[3] != '\0') {
		log_error("\nInvalid end address.\n");
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* -encr cipher:mode */
	if (hal_strncmp(MEMCRYPT_ENCR, argv[4], MEMCRYPT_ENCR_LEN) != 0) {
		log_error("\nMissing argument 3: %s.\n", MEMCRYPT_ENCR);
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}
	if (hal_strncmp(MEMCRPYT_AES128, argv[5], MEMCRPYT_AES128_LEN) == 0) {
		cipher = MCE_CIPHER_AES128;
		keysize = 16;
		mode = lib_strtoul(argv[5] + MEMCRPYT_AES128_LEN, &argv[5], 0);
		if (*argv[5] != 0) {
			log_error("\nInvalid encryption mode.\n");
			cmd_memcryptUsage(argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}
	else if (hal_strncmp(MEMCRPYT_AES256, argv[5], MEMCRYPT_AES256_LEN) == 0) {
		cipher = MCE_CIPHER_AES256;
		keysize = 32;
		mode = lib_strtoul(argv[5] + MEMCRYPT_AES256_LEN, &argv[5], 0);
		if (*argv[5] != 0) {
			log_error("\nInvalid encryption mode.\n");
			cmd_memcryptUsage(argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}
	else if (hal_strncmp(MEMCRPYT_NOEKEON, argv[5], MEMCRPYT_NOEKEON_LEN) == 0) {
		cipher = MCE_CIPHER_NOEKEON;
		keysize = 16;
		mode = lib_strtoul(argv[5] + MEMCRPYT_NOEKEON_LEN, &argv[5], 0);
		if (*argv[5] != 0) {
			log_error("\nInvalid encryption mode.\n");
			cmd_memcryptUsage(argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}
	else {
		log_error("\nUnknown cipher: %s\n", argv[5]);
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	/* -key k:val */
	if (hal_strncmp(MEMCRYPT_KEY, argv[6], MEMCRYPT_KEY_LEN) != 0) {
		log_error("\nMissing argument 5: %s.\n", MEMCRYPT_KEY);
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}
	if (hal_strncmp(MEMCRYPT_KEY_EXPLICIT, argv[7], MEMCRYPT_KEY_EXPLICIT_LEN) == 0) {
		ret = cmd_memcryptGetUserKey(argv[7] + MEMCRYPT_KEY_EXPLICIT_LEN, keysize, keyBuffer);
		if (ret < 0) {
			hal_memset(keyBuffer, 0, MEMCYPT_MAX_KEYSIZE);
			log_error("\nInvalid key format.\n");
			cmd_memcryptUsage(argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}
	else if (hal_strncmp(MEMCRYPT_KEY_OTP, argv[7], MEMCRYPT_KEY_OTP_LEN) == 0) {
		fuse = lib_strtol(argv[7] + MEMCRYPT_KEY_OTP_LEN, &argv[7], 0);
		if (*argv[7] != '\0') {
			log_error("\nInvalid fuse format.\n");
			cmd_memcryptUsage(argv[0]);
			return CMD_EXIT_FAILURE;
		}
		ret = cmd_memcryptGetOtpKey(fuse, keyBuffer, keysize);
		if (ret < 0) {
			hal_memset(keyBuffer, 0, MEMCYPT_MAX_KEYSIZE);
			log_error("\nInvalid OTP fuse: %d\n", fuse);
			cmd_memcryptUsage(argv[0]);
			return CMD_EXIT_FAILURE;
		}
	}
	else if (hal_strncmp(MEMCRYPT_KEY_RANDOM, argv[7], MEMCRYPT_KEY_RANDOM_LEN) == 0) {
		ret = _stm32_rngRead(keyBuffer, keysize);
		if (ret < 0) {
			hal_memset(keyBuffer, 0, MEMCYPT_MAX_KEYSIZE);
			log_error("\nFailed to generate random key\n");
			return CMD_EXIT_FAILURE;
		}
	}
	else {
		log_error("\nUnknown key option: %s\n", argv[7]);
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	args.cipher = cipher;
	args.mode = mode;
	args.start = start;
	args.end = end;
	args.key = keyBuffer;
	ret = devs_control(major, minor, DEV_CONTROL_MEMCRYPT, (void *)&args);
	if (ret < 0) {
		hal_memset(keyBuffer, 0, MEMCYPT_MAX_KEYSIZE);
		log_error("\nXSPI driver error: %d\n", ret);
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	hal_memset(keyBuffer, 0, MEMCYPT_MAX_KEYSIZE);
	return CMD_EXIT_SUCCESS;
}


static const cmd_t memcrypt_cmd __attribute__((section("commands"), used)) = {
	.name = "memcrypt", .run = cmd_memcrypt, .info = cmd_memcryptInfo
};
