/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Test device
 *
 * Copyright 2023 Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>
#include <syspage.h>
#include <devices/devs.h>

#include "cmd.h"


#define TEST_DEV_TIMEOUT_MS 500u

#define ERASE_BEFORE 1u
#define ERASE_AFTER  2u

#define BUF_SIZE 257 /* Write and read size specially not aligned to any power of 2 to possibly test more edge cases. */

#define CMD_TESTDEV_READ_BIG_BUF_SIZE 0 /* Used to perform a check with a read using big buffer. */


static void cmd_testDevInfo(void)
{
	lib_consolePuts("performs simple dev read/write test, usage:test-dev [-e erase before] [-E erase after] -d <dev> [-s <addr> start(default 0)] -l length");
}


static void cmd_testDevUsage(void)
{
	/* clang-format off */
	lib_consolePuts("Usage: test-dev [-e] [-E] -d <major>.<minor> -a <addr> [-l <length>]\n"
	"  -e       Erase before\n"
	"  -E       Erase after\n"
	"  -s       Start address, default: 0\n"
	"  -l       Length\n"
	);
	/* clang-format on */
}


static int cmd_testDevWrite(int major, int minor, addr_t addr, size_t length, u8 *buf, size_t buflen)
{
	ssize_t res;
	size_t rsz, i, chunk;

	for (rsz = 0; rsz < length; rsz += res) {
		chunk = min((length - rsz), buflen);
		for (i = 0; i < chunk; i++) {
			buf[i] = ((rsz + i) & 0xffu);
		}
		res = devs_write(major, minor, addr + rsz, buf, chunk);
		if (res < 0) {
			lib_printf("\nWrite failed %zd\n", res);
			return res;
		}
	}
	return EOK;
}


static int cmd_testDevRead(int major, int minor, addr_t addr, size_t length, u8 *buf, size_t buflen)
{
	ssize_t res;
	size_t rsz, chunk;

	lib_printf("\nRead device %d.%d  from 0x%x to 0x%x (%zu bytes):\n", major, minor, (u32)addr, (u32)addr + length, length);
	lib_consolePutHLine();

	for (rsz = 0; rsz < length; rsz += res) {
		chunk = min((length - rsz), buflen);
		res = devs_read(major, minor, addr + rsz, buf, chunk, TEST_DEV_TIMEOUT_MS);
		if (res < 0) {
			log_error("\nCan't read data %zd\n", res);
			return res;
		}
		lib_consolePutRegionHex((addr_t)buf, (addr_t)buf + res, addr + rsz, 0, NULL);
	}
	return EOK;
}


static int cmd_testDevErase(int major, int minor, addr_t addr, size_t length)
{
	ssize_t res;

	res = devs_erase(major, minor, addr, length, 0);
	if (res < 0) {
		log_error("\nErase failed %zd\n", res);
		return -EINVAL;
	}

	return EOK;
}


static int cmd_testDevCheck(int major, int minor, addr_t addr, size_t length, u8 *buf, size_t buflen)
{
	ssize_t res;
	size_t rsz, i, chunk;

	for (rsz = 0; rsz < length; rsz += res) {
		chunk = min((length - rsz), buflen);
		res = devs_read(major, minor, addr + rsz, buf, chunk, TEST_DEV_TIMEOUT_MS);
		if (res < 0) {
			log_error("\nCan't read data %zd\n", res);
			return res;
		}
		for (i = 0; i < chunk; i++) {
			if (buf[i] != ((rsz + i) & 0xffu)) {
				log_error("\nDiscrepancy at index %zu, expected %zu got %u\n", rsz + i, (rsz + i) & 0xffu, buf[i]);
				return -1;
			}
		}
	}
	return EOK;
}


static int cmd_doTestDev(int major, int minor, addr_t addr, size_t length, u8 erase)
{
	ssize_t res;
	static u8 buf[BUF_SIZE];
#if (CMD_TESTDEV_READ_BIG_BUF_SIZE != 0)
	static u8 bufBig[CMD_TESTDEV_READ_BIG_BUF_SIZE] __attribute__((section(".uncached_ddr")));
#endif
	log_setEcho(1);

	res = devs_check(major, minor);
	if (res < 0) {
		log_error("\nInvalid device %zd\n", res);
		return -EINVAL;
	}

	if ((erase & ERASE_BEFORE) != 0u) {
		res = cmd_testDevErase(major, minor, addr, length);
		if (res < 0) {
			return res;
		}
	}

	res = cmd_testDevWrite(major, minor, addr, length, buf, BUF_SIZE);
	if (res < 0) {
		if ((erase & ERASE_AFTER) != 0u) {
			(void)cmd_testDevErase(major, minor, addr, length);
		}

		return res;
	}

	res = devs_sync(major, minor);
	if (res < 0) {
		lib_printf("\nSync failed %zd\n", res);
		if ((erase & ERASE_AFTER) != 0u) {
			(void)cmd_testDevErase(major, minor, addr, length);
		}

		return res;
	}

	res = cmd_testDevCheck(major, minor, addr, length, buf, BUF_SIZE);
	if (res < 0) {
		(void)cmd_testDevRead(major, minor, addr, length, buf, BUF_SIZE);
		if ((erase & ERASE_AFTER) != 0u) {
			(void)cmd_testDevErase(major, minor, addr, length);
		}

		return res;
	}

#if (CMD_TESTDEV_READ_BIG_BUF_SIZE != 0)
	res = cmd_testDevCheck(major, minor, addr, length, bufBig, CMD_TESTDEV_READ_BIG_BUF_SIZE);
	if (res < 0) {
		(void)cmd_testDevRead(major, minor, addr, length, bufBig, CMD_TESTDEV_READ_BIG_BUF_SIZE);
		if ((erase & ERASE_AFTER) != 0u) {
			(void)cmd_testDevErase(major, minor, addr, length);
		}

		return res;
	}
#endif

	if ((erase & ERASE_AFTER) != 0u) {
		res = cmd_testDevErase(major, minor, addr, length);
		if (res < 0) {
			return res;
		}
	}

	log_info("\ntest-dev: Success!\n");

	return EOK;
}


static int cmd_testDev(int argc, char *argv[])
{
	int major = -1, minor = -1, opt, err;
	u8 erase = 0;
	char *endptr;
	addr_t start = 0;
	size_t length = -1;

	for (;;) {
		opt = lib_getopt(argc, argv, "eEd:s:l:");
		if (opt < 0) {
			break;
		}
		switch (opt) {
			case 'e':
				erase |= ERASE_BEFORE;
				break;

			case 'E':
				erase |= ERASE_AFTER;
				break;

			case 'd':
				major = lib_strtoul(optarg, &endptr, 0);
				if (*endptr != '.') {
					log_error("\nInvalid device.\n");
					cmd_testDevUsage();
					return CMD_EXIT_FAILURE;
				}
				minor = lib_strtoul(endptr + 1, &endptr, 0);
				if (*endptr != '\0') {
					log_error("\nInvalid device.\n");
					cmd_testDevUsage();
					return CMD_EXIT_FAILURE;
				}
				break;

			case 's':
				start = lib_strtoul(optarg, &endptr, 0);
				if (*endptr != '\0') {
					log_error("\nInvalid start.\n");
					cmd_testDevUsage();
					return CMD_EXIT_FAILURE;
				}
				break;

			case 'l':
				length = lib_strtoul(optarg, &endptr, 0);
				if ((*endptr != '\0') || (length == 0u)) {
					log_error("\nInvalid length.\n");
					cmd_testDevUsage();
					return CMD_EXIT_FAILURE;
				}
				break;

			default:
				cmd_testDevUsage();
				return CMD_EXIT_FAILURE;
		}
	}
	if (major == -1) {
		log_error("\nDevice missing.\n");
		return CMD_EXIT_FAILURE;
	}
	if (length == (size_t)-1) {
		log_error("\nLength missing.\n");
		return CMD_EXIT_FAILURE;
	}
	if ((start + length) < start) {
		log_error("\nStart + length causes overflow.\n");
		return CMD_EXIT_FAILURE;
	}

	err = cmd_doTestDev(major, minor, start, length, erase);
	if (err < 0) {
		log_error("\nError: %d\n", err);
		return CMD_EXIT_FAILURE;
	}
	return CMD_EXIT_SUCCESS;
}


__attribute__((constructor)) static void cmd_testDevReg(void)
{
	const static cmd_t app_cmd = { .name = "test-dev", .run = cmd_testDev, .info = cmd_testDevInfo };

	cmd_reg(&app_cmd);
}
