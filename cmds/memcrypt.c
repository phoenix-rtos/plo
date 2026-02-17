/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Set up memory encryption on device
 *
 * Copyright 2025, 2026 Phoenix Systems
 * Author: Krzysztof Radzewicz, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <devices/devs.h>
#include <hal/hal.h>
#include <lib/lib.h>

/* TODO: platform-independent OTP */
#include <hal/armv8m/stm32/n6/otp.h>

#define CONST_STR_LEN(x)          (sizeof(x) - 1)
#define MEMCRYPT_KEY_LEN          CONST_STR_LEN(MEMCRYPT_KEY)
#define MEMCRYPT_AES256           "aes256:"
#define MEMCRYPT_AES256_LEN       CONST_STR_LEN(MEMCRYPT_AES256)
#define MEMCRYPT_AES128           "aes128:"
#define MEMCRYPT_AES128_LEN       CONST_STR_LEN(MEMCRYPT_AES128)
#define MEMCRYPT_NOEKEON          "noekeon:"
#define MEMCRYPT_NOEKEON_LEN      CONST_STR_LEN(MEMCRYPT_NOEKEON)
#define MEMCRYPT_KEY_EXPLICIT     "x:"
#define MEMCRYPT_KEY_EXPLICIT_LEN CONST_STR_LEN(MEMCRYPT_KEY_EXPLICIT)
#define MEMCRYPT_KEY_RANDOM       "r:"
#define MEMCRYPT_KEY_RANDOM_LEN   CONST_STR_LEN(MEMCRYPT_KEY_RANDOM)
#define MEMCRYPT_KEY_OTP          "p:"
#define MEMCRYPT_KEY_OTP_LEN      CONST_STR_LEN(MEMCRYPT_KEY_OTP)
#define MEMCRYPT_KEY_DEV          "d:"
#define MEMCRYPT_KEY_DEV_LEN      CONST_STR_LEN(MEMCRYPT_KEY_DEV)

/* Unlikely we'll need more than 512 bits for the key */
#define MEMCRYPT_MAX_KEYSIZE 64

#define PARSE_HAVE_DEV      (1 << 0)
#define PARSE_HAVE_ENCR     (1 << 1)
#define PARSE_HAVE_KEY      (1 << 2)
#define PARSE_HAVE_REQUIRED ((1 << 3) - 1)

enum {
	buffer_invalid = 0,
	buffer_explicit,
	buffer_otp,
	buffer_device,
};


typedef struct {
	int type;
	unsigned int major;
	unsigned int minor;
	addr_t offset;
	size_t size;
	u8 data[MEMCRYPT_MAX_KEYSIZE];
} cmd_memcrypt_buffer_t;


typedef struct {
	unsigned int major;
	unsigned int minor;
	dev_memcrypt_args_t args;
	cmd_memcrypt_buffer_t key;
	cmd_memcrypt_buffer_t iv;
} cmd_memcrypt_params_t;


static void cmd_memcryptInfo(void)
{
	lib_printf("Configure external memory encryption. Use <-h> to see usage");
}


static void cmd_memcryptUsage(const char *name)
{
	lib_printf(
			"\nUsage: %s -d dev -p algo:mode -k <buf> [-i <buf>] start:end\n"
			"where:\n"
			"\t-d dev - encrypted storage device (major.minor)\n"
			"\t-p encryption settings\n"
			"\t\talgo - cipher: aes128, aes256, noekeon\n"
			"\t\tmode - mode of operation: integer, device dependent\n"
			"\t-k <buf> - encryption key\n"
			"\t-i <buf> - initial value, IV (optional)\n"
			"\n"
			"\t<buf> - buffer format: r:size|p:offs:size|d:dev:offs:size|x:hex\n"
			"\t\tr - generate random key\n"
			"\t\tp - use key stored in OTP memory\n"
			"\t\td - read from device\n"
			"\t\tx - provide key explicitly as sequence of bytes in hexadecimal\n"
			"\tstart - first offset to be encrypted\n"
			"\tend - first offset after the encrypted region\n",
			name);
}


static int cmd_memcrypt_consumePrefix(char **arg, char *prefix, size_t len)
{
	if (hal_strncmp(prefix, *arg, len) == 0) {
		*arg += len;
		return 1;
	}

	return 0;
}


/* Helper function to verify and convert whole string to number */
static int cmd_memcrypt_consumeNumber(char **arg, int endChar, unsigned long *out)
{
	char *end = NULL;
	*out = lib_strtoul(*arg, &end, 0);
	if ((end == *arg) || ((endChar != -1) && (*end != (char)endChar))) {
		return 0;
	}

	*arg = end + ((endChar != -1) ? 1 : 0);
	return 1;
}


static int cmd_memcrypt_consumeDevice(char **arg, unsigned int *major, unsigned int *minor)
{
	unsigned long tmpMajor, tmpMinor;

	if (!cmd_memcrypt_consumeNumber(arg, '.', &tmpMajor)) {
		return 0;
	}

	if (!cmd_memcrypt_consumeNumber(arg, -1, &tmpMinor)) {
		return 0;
	}

	*major = tmpMajor;
	*minor = tmpMinor;
	return 1;
}


static ssize_t cmd_memcrypt_getHexBuffer(char *string, u8 *out, size_t maxSize)
{
	char *end = NULL;
	size_t len;
	size_t i;
	unsigned long result;
	char tmp[3];

	/* We treat the string as hex regardless of prefix */
	if ((string[0] == '0') && ((string[1] | 0x20) == 'x')) {
		string += 2;
	}

	len = hal_strlen(string);
	if ((len == 0) || ((len % 2) != 0) || ((len / 2) > maxSize)) {
		return -EINVAL;
	}

	tmp[2] = '\0';
	for (i = 0; i < len; i += 2) {
		tmp[0] = string[i];
		tmp[1] = string[i + 1];
		result = lib_strtoul(tmp, &end, 16);
		if ((end != (tmp + 2)) || (result > 255)) {
			return -EINVAL;
		}

		out[i / 2] = result;
	}

	return len / 2;
}


static int cmd_memcrypt_parseEncr(char *arg, cmd_memcrypt_params_t *params)
{
	unsigned long mode;
	if (cmd_memcrypt_consumePrefix(&arg, MEMCRYPT_AES128, MEMCRYPT_AES128_LEN)) {
		params->args.algo = DEV_MEMCRYPT_ALGO_AES128;
	}
	else if (cmd_memcrypt_consumePrefix(&arg, MEMCRYPT_AES256, MEMCRYPT_AES256_LEN)) {
		params->args.algo = DEV_MEMCRYPT_ALGO_AES256;
	}
	else if (cmd_memcrypt_consumePrefix(&arg, MEMCRYPT_NOEKEON, MEMCRYPT_NOEKEON_LEN)) {
		params->args.algo = DEV_MEMCRYPT_ALGO_NOEKEON;
	}
	else {
		log_error("\nUnknown cipher: %s", arg);
		return CMD_EXIT_FAILURE;
	}

	if (!cmd_memcrypt_consumeNumber(&arg, '\0', &mode)) {
		log_error("\nInvalid encryption mode");
		return CMD_EXIT_FAILURE;
	}

	params->args.mode = mode;
	return CMD_EXIT_SUCCESS;
}


static const char *cmd_memcrypt_parseBuffer(char *arg, cmd_memcrypt_buffer_t *buf)
{
	ssize_t ret;
	unsigned long tmp;

	if (cmd_memcrypt_consumePrefix(&arg, MEMCRYPT_KEY_EXPLICIT, MEMCRYPT_KEY_EXPLICIT_LEN)) {
		buf->type = buffer_explicit;
		ret = cmd_memcrypt_getHexBuffer(arg, buf->data, sizeof(buf->data));
		if (ret < 0) {
			return "hex string";
		}

		buf->size = (size_t)ret;
	}
	else if (cmd_memcrypt_consumePrefix(&arg, MEMCRYPT_KEY_OTP, MEMCRYPT_KEY_OTP_LEN)) {
		buf->type = buffer_otp;
		if (!cmd_memcrypt_consumeNumber(&arg, ':', &tmp)) {
			return "offset";
		}

		buf->offset = tmp;
		if (!cmd_memcrypt_consumeNumber(&arg, '\0', &tmp)) {
			return "size";
		}

		buf->size = tmp;
	}
	else if (cmd_memcrypt_consumePrefix(&arg, MEMCRYPT_KEY_DEV, MEMCRYPT_KEY_DEV_LEN)) {
		buf->type = buffer_device;
		if (!cmd_memcrypt_consumeDevice(&arg, &buf->major, &buf->minor)) {
			return "device";
		}

		if (*arg != ':') {
			return "offset";
		}

		arg++;
		if (!cmd_memcrypt_consumeNumber(&arg, ':', &tmp)) {
			return "offset";
		}

		buf->offset = tmp;
		if (!cmd_memcrypt_consumeNumber(&arg, '\0', &tmp)) {
			return "size";
		}

		buf->size = tmp;
	}
	else if (cmd_memcrypt_consumePrefix(&arg, MEMCRYPT_KEY_RANDOM, MEMCRYPT_KEY_RANDOM_LEN)) {
		buf->type = buffer_device;
		buf->major = DEV_RNG;
		buf->minor = 0;
		buf->offset = 0;
		if (!cmd_memcrypt_consumeNumber(&arg, '\0', &tmp)) {
			return "size";
		}

		buf->size = tmp;
	}
	else {
		return "buffer type";
	}

	return NULL;
}


static int cmd_memcrypt_parseArgs(int argc, char *argv[], cmd_memcrypt_params_t *params)
{
	unsigned long int tmp;
	const char *errMsg;
	char *arg;
	int ret;
	u32 parseStatus = 0;

	for (;;) {
		int opt = lib_getopt(argc, argv, "d:p:k:i:h");
		if (opt < 0) {
			break;
		}

		switch (opt) {
			case 'd':
				ret = cmd_memcrypt_consumeDevice(&optarg, &params->major, &params->minor);
				if ((ret == 0) || (*optarg != '\0')) {
					log_error("\nInvalid device");
					return CMD_EXIT_FAILURE;
				}

				parseStatus |= PARSE_HAVE_DEV;
				break;

			case 'p':
				if (cmd_memcrypt_parseEncr(optarg, params) != CMD_EXIT_SUCCESS) {
					return CMD_EXIT_FAILURE;
				}

				parseStatus |= PARSE_HAVE_ENCR;
				break;

			case 'k':
				errMsg = cmd_memcrypt_parseBuffer(optarg, &params->key);
				if (errMsg != NULL) {
					log_error("\n-k: invalid %s", errMsg);
					return CMD_EXIT_FAILURE;
				}

				parseStatus |= PARSE_HAVE_KEY;
				break;

			case 'i':
				errMsg = cmd_memcrypt_parseBuffer(optarg, &params->iv);
				if (errMsg != NULL) {
					log_error("\n-i: invalid %s", errMsg);
					return CMD_EXIT_FAILURE;
				}

				break;

			case 'h': /* Fall-through */
			default:
				return CMD_EXIT_FAILURE;
		}
	}

	if (parseStatus != PARSE_HAVE_REQUIRED) {
		log_error("\nSome options are missing");
		return CMD_EXIT_FAILURE;
	}

	if (argc < (optind + 1)) {
		log_error("\nStart and end address missing");
		return CMD_EXIT_FAILURE;
	}

	arg = argv[optind];
	if (cmd_memcrypt_consumeNumber(&arg, ':', &tmp) == 0) {
		log_error("\nStart address invalid");
		return CMD_EXIT_FAILURE;
	}

	params->args.start = tmp;

	if (cmd_memcrypt_consumeNumber(&arg, '\0', &tmp) == 0) {
		log_error("\nEnd address invalid");
		return CMD_EXIT_FAILURE;
	}

	params->args.end = tmp;
	return CMD_EXIT_SUCCESS;
}


static int cmd_memcrypt_readOtpBuffer(int fuse, u8 *out, size_t keysize)
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

		out[i] = val & 0xffUL;
		out[i + 1] = (val >> 8) & 0xffUL;
		out[i + 2] = (val >> 16) & 0xffUL;
		out[i + 3] = (val >> 24) & 0xffUL;
		fuse += 1;
		i += 4;
	}

	return EOK;
}


static int cmd_memcrypt_readBuffer(cmd_memcrypt_buffer_t *buf, size_t expectedSize)
{
	int ret;
	if (expectedSize != 0) {
		if (buf->size != expectedSize) {
			log_error("\nInvalid key size.");
			return CMD_EXIT_FAILURE;
		}
	}

	if (buf->size > sizeof(buf->data)) {
		log_error("\nRequested key too large.");
		return CMD_EXIT_FAILURE;
	}

	switch (buf->type) {
		case buffer_explicit:
			/* Key already in buffer */
			break;

		case buffer_otp:
			ret = cmd_memcrypt_readOtpBuffer(buf->offset, buf->data, buf->size);
			if (ret < 0) {
				log_error("\nFailed to read OTP fuse");
				return CMD_EXIT_FAILURE;
			}

			break;

		case buffer_device:
			ret = devs_read(buf->major, buf->minor, buf->offset, buf->data, buf->size, -1);
			if (ret < 0) {
				log_error("\nFailed to read key");
				return CMD_EXIT_FAILURE;
			}

			break;

		default:
			return CMD_EXIT_FAILURE;
	}

	return CMD_EXIT_SUCCESS;
}


static int _cmd_memcrypt(int argc, char *argv[], cmd_memcrypt_params_t *params)
{
	int ret;
	u32 keySize;

	/* Memcrypt only checks if device number is correct and parses string arguments.
	 * Argument validity is checked by the device. */
	if (cmd_memcrypt_parseArgs(argc, argv, params) != CMD_EXIT_SUCCESS) {
		cmd_memcryptUsage(argv[0]);
		return CMD_EXIT_FAILURE;
	}

	if (params->major != DEV_CRYP_STORAGE) {
		log_error("\nDevice major must be %d (DEV_CRYP_STORAGE)", DEV_CRYP_STORAGE);
		return -EINVAL;
	}

	ret = devs_check(params->major, params->minor);
	if (ret < 0) {
		log_error("\nDevice %d.%d not available", params->major, params->minor);
		return -EINVAL;
	}

	switch (params->args.algo) {
		case DEV_MEMCRYPT_ALGO_AES128: keySize = 16; break;
		case DEV_MEMCRYPT_ALGO_AES256: keySize = 32; break;
		case DEV_MEMCRYPT_ALGO_NOEKEON: keySize = 16; break;
		default: return CMD_EXIT_FAILURE;
	}

	if (cmd_memcrypt_readBuffer(&params->key, keySize) != CMD_EXIT_SUCCESS) {
		return CMD_EXIT_FAILURE;
	}

	params->args.key = params->key.data;
	if (params->iv.type != buffer_invalid) {
		if (cmd_memcrypt_readBuffer(&params->iv, 0) != CMD_EXIT_SUCCESS) {
			return CMD_EXIT_FAILURE;
		}

		params->args.iv = params->iv.data;
		params->args.ivSize = params->iv.size;
	}
	else {
		params->args.iv = NULL;
		params->args.ivSize = 0;
	}

	ret = devs_control(params->major, params->minor, DEV_CONTROL_MEMCRYPT, &params->args);
	if (ret < 0) {
		log_error("\nFailed to set up encryption: %d", ret);
		return CMD_EXIT_FAILURE;
	}

	return CMD_EXIT_SUCCESS;
}


static int cmd_memcrypt(int argc, char *argv[])
{
	int ret;
	static cmd_memcrypt_params_t params;
	/* Zero out the structure to have sane default parameters */
	hal_memset(&params, 0, sizeof(params));
	ret = _cmd_memcrypt(argc, argv, &params);
	/* To prevent possible key leaks, we zero out the structure after function return */
	hal_memset(&params, 0, sizeof(params));
	return ret;
}


static const cmd_t memcrypt_cmd __attribute__((section("commands"), used)) = {
	.name = "memcrypt", .run = cmd_memcrypt, .info = cmd_memcryptInfo
};
