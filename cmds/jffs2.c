/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Flash tool
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


#define JFFS2_MAGIC_BITMASK 0x1985

#define JFFS2_NODETYPE_CLEANMARKER 0x2003

#define MIN_CLEANMARKER_SIZE 12u


struct jffs2_cleanmarker_node {
	u16 magic;
	u16 nodetype;
	u32 totlen;
	u32 hdr_crc;
};


static void cmd_jffs2Info(void)
{
	lib_consolePuts("writes jffs2 cleanmarkers, usage: jffs2 -d <major>.<minor> -c <start block>:<number of blocks>:<block size>:<clean marker size> [-e]");
}


static void cmd_jffs2Usage(void)
{
	lib_consolePuts("Usage: jffs2 -d <major>.<minor> -c <start block>:<number of blocks>:<block size>:<clean marker size> [-e erase blocks]\n");
}


static int cmd_jffs2(int argc, char *argv[])
{
	int major = -1, minor = -1, opt, err, cleanmarkers = 0, erase = 0;
	unsigned long blockSize = -1, numBlocks = -1, startBlock = -1, cleanmarkerSize = -1, i;
	char *endptr;
	struct jffs2_cleanmarker_node cleanmarker;

	for (;;) {
		opt = lib_getopt(argc, argv, "c:d:e");
		if (opt < 0) {
			break;
		}
		switch (opt) {
			case 'e':
				erase = 1;
				break;
			case 'c':
				cleanmarkers = 1;

				startBlock = lib_strtoul(optarg, &endptr, 0);
				if (*endptr != ':') {
					log_error("\nInvalid startBlock.\n");
					cmd_jffs2Usage();
					return CMD_EXIT_FAILURE;
				}

				numBlocks = lib_strtoul(endptr + 1, &endptr, 0);
				if ((*endptr != ':')) {
					log_error("\nInvalid number of blocks.\n");
					cmd_jffs2Usage();
					return CMD_EXIT_FAILURE;
				}

				blockSize = lib_strtoul(endptr + 1, &endptr, 0);
				if ((*endptr != ':')) {
					log_error("\nInvalid block size.\n");
					cmd_jffs2Usage();
					return CMD_EXIT_FAILURE;
				}

				cleanmarkerSize = lib_strtoul(endptr + 1, &endptr, 0);
				if ((*endptr != '\0')) {
					log_error("\nInvalid clean marker size.\n");
					cmd_jffs2Usage();
					return CMD_EXIT_FAILURE;
				}
				break;

			case 'd':
				major = lib_strtoul(optarg, &endptr, 0);
				if (*endptr != '.') {
					log_error("\nInvalid device.\n");
					cmd_jffs2Usage();
					return CMD_EXIT_FAILURE;
				}
				minor = lib_strtoul(endptr + 1, &endptr, 0);
				if (*endptr != '\0') {
					log_error("\nInvalid device.\n");
					cmd_jffs2Usage();
					return CMD_EXIT_FAILURE;
				}
				break;

			default:
				cmd_jffs2Usage();
				return CMD_EXIT_FAILURE;
		}
	}

	if (major == -1) {
		log_error("\nDevice missing.\n");
		return CMD_EXIT_FAILURE;
	}

	if (cleanmarkers != 1) {
		log_error("\nNo action was chosen.\n");
		return CMD_EXIT_FAILURE;
	}

	if (cleanmarkerSize < MIN_CLEANMARKER_SIZE) {
		log_error("\nInvalid clean marker size. Minimal is %d.\n", MIN_CLEANMARKER_SIZE);
		return CMD_EXIT_FAILURE;
	}

	err = devs_check(major, minor);
	if (err < 0) {
		log_error("\nInvalid device %d\n", err);
		return -1;
	}

	cleanmarker.magic = JFFS2_MAGIC_BITMASK;
	cleanmarker.nodetype = JFFS2_NODETYPE_CLEANMARKER;
	/* TODO handle NAND memory */
	cleanmarker.totlen = cleanmarkerSize;

	cleanmarker.hdr_crc = lib_crc32((const u8 *)&cleanmarker, sizeof(struct jffs2_cleanmarker_node) - 4u, 0);

	lib_printf("\n");
	for (i = 0; i < numBlocks; i++) {
		/* FIXME should be: log_info("\rjffs2: block %lu/%lu", i, numBlocks); */
		lib_printf("\rjffs2: block %lu/%lu", i, numBlocks);

		if (erase != 0) {
			err = devs_erase(major, minor, (startBlock + i) * blockSize, blockSize, 0);
			if (err < 0) {
				log_error("\nError erasing %d\n", err);
			}
		}
		err = devs_write(major, minor, (startBlock + i) * blockSize, (const void *)&cleanmarker, sizeof(cleanmarker));
		if (err < 0) {
			log_error("\nError writing %d\n", err);
		}
	}

	log_info("\njffs2: written cleanmarks");

	return CMD_EXIT_SUCCESS;
}

__attribute__((constructor)) static void cmd_jffs2Reg(void)
{
	static const cmd_t app_cmd = { .name = "jffs2", .run = cmd_jffs2, .info = cmd_jffs2Info };

	cmd_reg(&app_cmd);
}
