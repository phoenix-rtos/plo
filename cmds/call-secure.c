/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Secure call loader script
 *
 * Copyright 2025 Phoenix Systems
 * Author: Hubert Buczynski, Krzysztof Radzewicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/armv8m/stm32/n6/pka.h>
#include <hal/armv8m/stm32/n6/hash.h>

#include <hal/hal.h>
#include <phfs/phfs.h>
#include <lib/lib.h>

#define P256_CURVE        "P-256"
#define SHA2_256          "sha2-256"
#define PRIVATE_KEY_BITS  256
#define PRIVATE_KEY_BYTES 32

#define SCRIPT_SZ 0x800


static void cmd_callsecureInfo(void)
{
	lib_printf("calls user's secure script, usage: call_secure <dev> <script name> <ecdsa curve> <hash algorithm> <public key>");
}


static int base64_value(char c)
{
	if (lib_islower(c)) {
		return c - 'a' + 26;
	}
	if (lib_isupper(c)) {
		return c - 'A';
	}
	if (lib_isdigit(c)) {
		return c - '0' + 52;
	}
	if (c == '+') {
		return 62;
	}
	if (c == '/') {
		return 63;
	}
	return -EINVAL;
}


static int base64_decode(const char *data, size_t datasize, u8 *out, size_t outsize)
{
	u32 v = 0;
	size_t i, j;
	if ((datasize % 4) != 0) {
		return -EINVAL;
	}
	if (outsize > (3 * datasize / 4)) {
		return -EINVAL;
	}
	if (outsize < (3 * datasize / 4 - 3)) {
		return -EINVAL;
	}
	for (i = 0, j = 0; i < datasize; i++) {
		if (data[i] == '=') {
			/* Ingore the data that comes after the first '=' */
			if ((i % 4) == 0) {
				/* Unnecessary padding */
				return -EINVAL;
			}
			i = 3;
			while (j < outsize) {
				*(out + (j++)) = (v >> (8 * (--i))) & 0xff;
			}
			break;
		}
		int val = base64_value(data[i]);
		if (val < 0) {
			return -EINVAL;
		}
		v |= ((val & 0x3f) << (18 - 6 * (i % 4)));
		if ((i % 4) == 3) {
			*(out + (j++)) = (v >> 16) & 0xff;
			*(out + (j++)) = (v >> 8) & 0xff;
			*(out + (j++)) = v & 0xff;
			v = 0;
		}
	}

	return 0;
}


static ssize_t readLine(handler_t h, addr_t *offs, char *buff, size_t maxsize)
{
	ssize_t len;
	ssize_t i = 0;
	char c;
	do {
		len = phfs_read(h, *offs, &c, 1);
		if (len < 0) {
			return len;
		}

		*offs += len;
		if ((len == 0) || (c == '\n') || (c == '\0' /* EOF */)) {
			buff[i] = '\0';
			if (c == '\0') {
				/* end of script */
				return 0;
			}

			return i;
		}

		if (i == (maxsize - 1)) {
			return -ENOMEM;
		}

		buff[i++] = c;
	} while (1);
}


static ssize_t readAll(handler_t h, addr_t *offs, char *buff, size_t maxsize)
{
	ssize_t len;
	size_t i = 0;
	char c;
	do {
		if (i >= maxsize) {
			return -ENOMEM;
		}

		len = phfs_read(h, *offs, &c, 1);
		if (len < 0) {
			return len;
		}

		*offs += len;
		if ((len == 0) || (c == '\0' /* EOF */)) {
			buff[i] = '\0';

			return i;
		}

		buff[i++] = c;
	} while (1);
}


static int cmd_callsecure(int argc, char *argv[])
{
	size_t i, j;
	ssize_t len;
	int res;
	handler_t h;
	addr_t offs = 0;
	size_t privkeyb64Len = ((PRIVATE_KEY_BYTES + 3 - 1) / 3) * 4; /* Expected lenth of the private key in base64 */
	char buff[SIZE_CMD_ARG_LINE];
	u8 pubkeyX[PRIVATE_KEY_BYTES];
	u8 pubkeyY[PRIVATE_KEY_BYTES];
	u8 sigR[PRIVATE_KEY_BYTES];
	u8 sigS[PRIVATE_KEY_BYTES];
	u8 hash[PRIVATE_KEY_BYTES];

	/* Entire script needs to be loaded into memory to avoid time of check vs time of use attack.
	 * It doesn't matter that the header and signature are loaded separately, because before execution
	 * the data and signature are verified with the provided public key. */

	static char script[SCRIPT_SZ] = {};
	for (int i = 0; i < SIZE_CMD_ARG_LINE; i++) {
		buff[i] = 0;
	}

	if (argc != 6) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}
	/* Check curve name validity. Don't return validation errors as there might be more user scripts */
	if (hal_strcmp(argv[3], P256_CURVE) != 0) {
		log_error("\nEliptic curve: %s not supported.", argv[3]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	/* Check hash algo name validity */
	if (hal_strcmp(argv[4], SHA2_256) != 0) {
		log_error("\nHash algorithm: %s not supported.", argv[4]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	if (hal_strlen(argv[5]) < 2 * privkeyb64Len) {
		log_error("\nInvalid public key length, expected: %d", 2 * privkeyb64Len);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	res = base64_decode(argv[5], privkeyb64Len, pubkeyX, PRIVATE_KEY_BYTES);
	if (res < 0) {
		log_error("\nInvalid base64 format %s", argv[5]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	res = base64_decode(argv[5] + privkeyb64Len, privkeyb64Len, pubkeyY, PRIVATE_KEY_BYTES);
	if (res < 0) {
		log_error("\nInvalid base64 format %s", argv[5]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}

	/* ARG_1: device name - argv[1]
	 * ARG_2: script name - argv[2] */
	res = phfs_open(argv[1], argv[2], 0, &h);
	if (res < 0) {
		log_error("\nCan't open %s, on %s", argv[2], argv[1]);
		return res;
	}

	/* Begin hash operation */
	(void)hash_initDigest(HASH_ALGO_SHA2_256);

	/* ARG_3: ecdsa curve name */
	len = readLine(h, &offs, buff, SIZE_CMD_ARG_LINE);
	if (len <= 0) {
		log_error("\nlen: %d\n", len);
		log_error("\nCan't read <ecdsa curve> from %s", argv[1]);
		phfs_close(h);
		return (len < 0) ? len : -EIO;
	}
	/* Check script ecdsa algorithm */
	if (hal_strcmp(buff, argv[3]) != 0) {
		log_error("\nInvalid elliptic curve: %s. Expected: %s.", buff, argv[3]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	/* Replace '\0' with '\n' (original value in partition) and feed the hash digest */
	buff[len] = '\n';
	(void)hash_feedMessage((const u8 *)buff, len + 1);

	/* ARG_4: hash algorithm name */
	len = readLine(h, &offs, buff, SIZE_CMD_ARG_LINE);
	if (len <= 0) {
		log_error("\nCan't read <hash algorithm> from %s", argv[1]);
		phfs_close(h);
		return (len < 0) ? len : -EIO;
	}
	/* Check script hash algorithm */
	if (hal_strcmp(buff, argv[4]) != 0) {
		log_error("\nInvalid hash algorithm: %s. Expected: %s.", buff, argv[4]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	buff[len] = '\n';
	(void)hash_feedMessage((const u8 *)buff, len + 1);

	/* ARG_5: ecdsa signature parts in base64 */
	len = readLine(h, &offs, buff, SIZE_CMD_ARG_LINE);
	if (len <= 0) {
		log_error("\nCan't read ecdsa signature from %s", argv[1]);
		phfs_close(h);
		return (len < 0) ? len : -EIO;
	}
	res = base64_decode(buff, privkeyb64Len, sigR, PRIVATE_KEY_BYTES);
	if (res < 0) {
		log_error("\nInvalid base64 format %s", buff);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	len = readLine(h, &offs, buff, SIZE_CMD_ARG_LINE);
	if (len <= 0) {
		log_error("\nCan't read ecdsa signature from %s", buff);
		phfs_close(h);
		return (len < 0) ? len : -EIO;
	}
	res = base64_decode(buff, privkeyb64Len, sigS, PRIVATE_KEY_BYTES);
	if (res < 0) {
		log_error("\nInvalid base64 format %s", argv[5]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	/* Signature is not fed into the hash */

	/* Read entire script and calculate hash */
	len = readAll(h, &offs, script, SCRIPT_SZ);
	if (len < 0) {
		log_error("\nCan't read script contents %s", argv[1]);
		phfs_close(h);
		return (len < 0) ? len : -EIO;
	}
	/* The ending '\0' byte is also fed into the hash */
	(void)hash_feedMessage((const u8 *)script, len + 1);
	(void)hash_getDigest(hash, PRIVATE_KEY_BYTES);

	res = pka_ecdsaVerify(sigR, sigS, hash, pubkeyX, pubkeyY, PRIVATE_KEY_BITS);
	if (res < 0) {
		log_error("\n%s: %s verification failed.", argv[0], argv[2]);
		phfs_close(h);
		return CMD_EXIT_SUCCESS;
	}
	else {
		lib_printf("\n%s: %s verified\n", argv[0], argv[2]);
	}

	/* Execute script */
	i = 0, j = 0;
	lib_printf(CONSOLE_NORMAL);
	do {
		/* Find '\n' and replace with '0' */
		if ((script[i] == '\n') || (script[i] == '\0' /* EOF */)) {
			if (script[i] == '\0') {
				/* end of script */
				break;
			}

			buff[j] = '\0';
			res = cmd_parse(buff);
			if (res != CMD_EXIT_SUCCESS) {
				return (res < 0) ? res : -EINVAL;
			}
			j = 0;
			i++;

			continue;
		}

		if (j == (sizeof(buff) - 1)) {
			log_error("\nLine in %s exceeds buffer size", argv[2]);
			phfs_close(h);
			return -ENOMEM;
		}

		buff[j++] = script[i++];
	} while (i < len + 1);

	phfs_close(h);
	return CMD_EXIT_SUCCESS;
}


static const cmd_t call_cmd __attribute__((section("commands"), used)) = {
	.name = "call-secure", .run = cmd_callsecure, .info = cmd_callsecureInfo
};
