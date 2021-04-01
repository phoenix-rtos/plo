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
#include "../cmd.h"
#include "../phfs.h"
#include "../hal.h"
#include "../cmd.h"

#include "zynq.h"
#include "config.h"
#include "cmd-board.h"


void cmd_bitstream(char *s)
{
	int res;
	u8 buff[384];
	u32 offs = 0;
	handle_t handle;
	unsigned int pdn;
	addr_t addr = BISTREAM_ADDR;

	u16 argsc = 0;
	char args[MAX_CMD_ARGS_NB][LINESZ + 1];

	if ((cmd_parseArgs(s, args, &argsc) < 0) || argsc > 2) {
		plostd_printf(ATTR_ERROR, "\nWrong arguments!!\n");
		return;
	}

	if (cmd_checkDev(args[0], &pdn) < 0)
		return;

	plostd_printf(ATTR_LOADER, "\nOpening device %s\n", args[0]);
	handle = phfs_open(pdn, args[1], 0);
	if (handle.h < 0) {
		plostd_printf(ATTR_ERROR, "Cannot initialize device: %s\n", args[0]);
		return;
	}

	plostd_printf(ATTR_LOADER, "Loading bitstream into DDR\n");
	do {
		if ((res = phfs_read(pdn, handle, (addr_t *)&offs, buff, sizeof(buff))) < 0) {
			plostd_printf(ATTR_ERROR, "\nCan't read segment data\n");
			return;
		}

		hal_memcpy((void *)addr, buff, res);
		addr += res;
	} while (res != 0);

	plostd_printf(ATTR_LOADER, "Loading bitstream into PL\n");
	if (_zynq_loadPL(BISTREAM_ADDR, addr - BISTREAM_ADDR) < 0) {
		plostd_printf(ATTR_ERROR, "PL was not initialized, bitstream is incorrect\n");
		return;
	}

	plostd_printf(ATTR_LOADER, "PL was successfully initialized\n");
}
