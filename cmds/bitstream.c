/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Load bitsream to fpga
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/hal.h>
#include <phfs/phfs.h>


static void cmd_bistreamInfo(void)
{
	lib_printf("loads bitstream into PL, usage:\n");
	lib_printf("%17s%s", "", "bitstream <dev> <name>");
}


static int cmd_bitstream(char *s)
{
	int res;
	handler_t handler;
	u8 buff[SIZE_MSG_BUFF];
	addr_t offs = 0, addr = BITSREAM_ADDR;

	unsigned int argsc;
	cmdarg_t *args;

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	if (argsc == 0) {
		log_error("\nArguments have to be defined");
		return -EINVAL;
	}
	else if (argsc != 2) {
		log_error("\nWrong args: %s", s);
		return -EINVAL;
	}


	if (phfs_open(args[0], args[1], 0, &handler) < 0) {
		log_error("\nCan't open %s, on %s", args[1], args[0]);
		return -EINVAL;
	}

	log_info("\nLoading bitstream into DDR, please wait...");
	do {
		if ((res = phfs_read(handler, offs, buff, sizeof(buff))) < 0) {
			log_error("\nCan't read %s from %s", args[1], args[0]);
			return -EINVAL;
		}

		hal_memcpy((void *)addr, buff, res);
		addr += res;
		offs += res;
	} while (res != 0);

	log_info("\nLoading bitstream into PL");
	if (_zynq_loadPL(BITSREAM_ADDR, addr - BITSREAM_ADDR) < 0) {
		log_error("\nPL was not initialized, bitstream is incorrect");
		return -EINVAL;
	}

	log_info("\nPL was successfully initialized");

	return EOK;
}


__attribute__((constructor)) static void cmd_appreg(void)
{
	const static cmd_t app_cmd = { .name = "bitstream", .run = cmd_bitstream, .info = cmd_bistreamInfo };

	cmd_reg(&app_cmd);
}
