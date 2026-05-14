/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Verify CRC32 of a loaded program
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cmd.h"

#include <hal/hal.h>
#include <lib/lib.h>
#include <lib/crc32.h>
#include <syspage.h>


static void cmd_verifycrc32Info(void)
{
	lib_printf("verifies CRC32 of a loaded program, usage: verify-crc32 <name> <crc32>");
}


static int cmd_verifycrc32(int argc, char *argv[])
{
	const syspage_prog_t *prog;
	u32 expectedCrc, computedCrc;
	char *endptr;

	if (argc != 3) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return CMD_EXIT_FAILURE;
	}

	prog = syspage_progFind(argv[1]);
	if (prog == NULL) {
		log_error("\n%s: Program '%s' not found", argv[0], argv[1]);
		return CMD_EXIT_FAILURE;
	}

	expectedCrc = (u32)lib_strtoul(argv[2], &endptr, 0);
	if ((argv[2] == endptr) || (*endptr != '\0')) {
		log_error("\n%s: Invalid CRC32 value '%s'", argv[0], argv[2]);
		return CMD_EXIT_FAILURE;
	}

	computedCrc = lib_crc32((const u8 *)prog->start, prog->end - prog->start, 0xffffffffU) ^ 0xffffffffU;

	if (computedCrc != expectedCrc) {
		log_error("\n%s: CRC mismatch for '%s' (expected 0x%08x, got 0x%08x)", argv[0], argv[1], expectedCrc, computedCrc);
		return CMD_EXIT_FAILURE;
	}

	log_info("\n%s: CRC32 OK for '%s'", argv[0], argv[1]);

	return CMD_EXIT_SUCCESS;
}


static const cmd_t verifycrc32_cmd __attribute__((section("commands"), used)) = {
	.name = "verify-crc32", .run = cmd_verifycrc32, .info = cmd_verifycrc32Info
};
