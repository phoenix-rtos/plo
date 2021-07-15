/*
 * Phoenix-RTOS
 *
 * Operating system loader
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


static int cmd_bitstream(int argc, char *argv[])
{
	ssize_t res;
	handler_t handler;
	u8 buff[SIZE_MSG_BUFF];
	addr_t offs = 0, addr = ADDR_BITSTREAM;

	if (argc == 1) {
		log_error("\n%s: Arguments have to be defined", argv[0]);
		return -EINVAL;
	}
	else if (argc != 3) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}


	if ((res = phfs_open(argv[1], argv[2], 0, &handler)) < 0) {
		log_error("\nCan't open %s, on %s", argv[2], argv[1]);
		return res;
	}

	log_info("\nLoading bitstream into DDR, please wait...");
	do {
		if ((res = phfs_read(handler, offs, buff, sizeof(buff))) < 0) {
			log_error("\nCan't read %s from %s", argv[2], argv[1]);
			return res;
		}

		hal_memcpy((void *)addr, buff, res);
		addr += res;
		offs += res;
	} while (res != 0);

	log_info("\nLoading bitstream into PL");
	if ((res = _zynq_loadPL(ADDR_BITSTREAM, addr - ADDR_BITSTREAM)) < 0) {
		log_error("\nPL was not initialized, bitstream is incorrect");
		return res;
	}

	log_info("\nPL was successfully initialized");

	return EOK;
}


__attribute__((constructor)) static void cmd_appreg(void)
{
	const static cmd_t app_cmd = { .name = "bitstream", .run = cmd_bitstream, .info = cmd_bistreamInfo };

	cmd_reg(&app_cmd);
}
