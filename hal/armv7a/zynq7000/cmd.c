/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader commands
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../errors.h"

#include "hal.h"
#include "cmd.h"
#include "lib.h"
#include "zynq.h"
#include "phfs.h"
#include "config.h"
#include "cmd-board.h"


void cmd_bitstream(char *s)
{
	int res;
	u8 buff[384];
	addr_t offs = 0;
	handler_t handler;
	addr_t addr = BISTREAM_ADDR;

	u16 argsc = 0;
	char args[MAX_CMD_ARGS_NB][LINESZ + 1];

	if ((cmd_parseArgs(s, args, &argsc) < 0) || argsc > 2) {
		lib_printf("\nWrong arguments!!\n");
		return;
	}

	if (phfs_open(args[0], args[1], 0, &handler) < 0) {
		lib_printf("Cannot initialize device: %s\n", args[0]);
		return;
	}

	lib_printf("\nLoading bitstream into DDR\n");
	do {
		if ((res = phfs_read(handler, offs, buff, sizeof(buff))) < 0) {
			lib_printf("\nCan't read segment data\n");
			return;
		}

		hal_memcpy((void *)addr, buff, res);
		addr += res;
		offs += res;
	} while (res != 0);

	lib_printf("Loading bitstream into PL\n");
	if (_zynq_loadPL(BISTREAM_ADDR, addr - BISTREAM_ADDR) < 0) {
		lib_printf("PL was not initialized, bitstream is incorrect\n");
		return;
	}

	lib_printf("PL was successfully initialized\n");
}
