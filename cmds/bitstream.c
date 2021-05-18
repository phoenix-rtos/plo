/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * Load bistream to fpga
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"
#include "hal.h"
#include "zynq.h"
#include "elf.h"
#include "phfs.h"


static void cmd_bistreamInfo(void)
{
	lib_printf("loads bitstream into PL, usage:\n");
	lib_printf("%17s%s", "", "bitstream [<boot device>] [<name>]");
}


static int cmd_bitstream(char *s)
{
	int res;
	handler_t handler;
	u8 buff[SIZE_MSG_BUFF];
	addr_t offs = 0, addr = BISTREAM_ADDR;

	unsigned int argsc;
	char (*args)[SIZE_CMD_ARG_LINE];

	if ((argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args)) != 2) {
		log_error("\nWrong args: %s", s);
		return ERR_ARG;
	}

	if (phfs_open(args[0], args[1], 0, &handler) < 0) {
		log_error("\nCan't open %s, on %s", args[1], args[0]);
		return ERR_ARG;
	}

	log_info("\nLoading bitstream into DDR, please wait...\n");
	do {
		if ((res = phfs_read(handler, offs, buff, sizeof(buff))) < 0) {
			log_error("\nCan't read %s from %s", args[1], args[0]);
			return ERR_ARG;
		}

		hal_memcpy((void *)addr, buff, res);
		addr += res;
		offs += res;
	} while (res != 0);

	log_info("\nLoading bitstream into PL\n");
	if (_zynq_loadPL(BISTREAM_ADDR, addr - BISTREAM_ADDR) < 0) {
		log_error("\nPL was not initialized, bitstream is incorrect\n");
		return ERR_ARG;
	}

	log_info("\nPL was successfully initialized\n");

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_appreg(void)
{
	const static cmd_t app_cmd = { .name = "bistream", .run = cmd_bitstream, .info = cmd_bistreamInfo };

	cmd_reg(&app_cmd);
}
