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
	u8 buff[SIZE_MSG_BUFF];
	addr_t offs = 0, addr = BISTREAM_ADDR;
	handler_t handler;

	u16 argsc = 0;
	char args[2][SIZE_CMD_ARG_LINE + 1];

	if ((cmd_parseArgs(s, args, &argsc, 2) < 0) || argsc > 2) {
		log_error("\ncmd/bistream: Wrong arguments");
		return ERR_ARG;
	}

	if (phfs_open(args[0], args[1], 0, &handler) < 0) {
		log_error("\ncmd/bistream: Cannot initialize device: %s", args[0]);
		return ERR_ARG;
	}

	log_info("\ncmd/bistream: Loading bitstream into DDR\n");
	do {
		if ((res = phfs_read(handler, offs, buff, sizeof(buff))) < 0) {
			log_error("\ncmd/bistream: Can't read segment data");
			return ERR_ARG;
		}

		hal_memcpy((void *)addr, buff, res);
		addr += res;
		offs += res;
	} while (res != 0);

	log_info("\ncmd/bistream: Loading bitstream into PL\n");
	if (_zynq_loadPL(BISTREAM_ADDR, addr - BISTREAM_ADDR) < 0) {
		log_error("\ncmd/bistream: PL was not initialized, bitstream is incorrect\n");
		return ERR_ARG;
	}

	log_info("\ncmd/bistream: PL was successfully initialized\n");

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_appreg(void)
{
	const static cmd_t app_cmd = { .name = "bistream", .run = cmd_bitstream, .info = cmd_bistreamInfo };

	cmd_reg(&app_cmd);
}
