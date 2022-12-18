/*
 * Phoenix-RTOS
 *
 * i.MX RT10XX/117X OTP tool
 *
 * Reads or writes OCOTP fuses
 *
 * Copyright 2020-2022 Phoenix Systems
 * Author: Aleksander Kaminski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "cmd.h"

#include <hal/armv7m/imxrt/otp.h>
#include <lib/lib.h>


static void cmd_otpInfo(void)
{
	lib_printf("Reads or writes On-Chip OTP registers: otp <-r|-w value> <-f fuse>");
}


static void cmd_otpUsage(void)
{
	lib_printf("Usage: otp [-r | -w value] -f fuse\n"
			   "  -r       Read fuse\n"
			   "  -w val   Write fuse with value [val] (32-bit number)\n"
			   "  -f num   Select fuse (num) to be read or written\n");
}


int cmd_otp(int argc, char *argv[])
{
	int opt, res, read = 0, write = 0, fuse = -1;
	u32 val = 0;

	lib_printf("\n");

	while ((opt = lib_getopt(argc, argv, "f:rw:")) >= 0) {
		switch (opt) {
			case 'f':
				fuse = (int)lib_strtol(optarg, NULL, 0);
				break;

			case 'r':
				read = 1;
				break;

			case 'w':
				write = 1;
				val = (u32)lib_strtoul(optarg, NULL, 0);
				break;

			default:
				cmd_otpUsage();
				return CMD_EXIT_FAILURE;
		}
	}

	if ((read ^ write) == 0) {
		lib_printf("Specify either read or write mode\n");
		cmd_otpUsage();
		return CMD_EXIT_FAILURE;
	}

	if (otp_checkFuseValid(fuse) < 0) {
		lib_printf("Invalid fuse number\n");
		return CMD_EXIT_FAILURE;
	}

	if (read != 0) {
		res = otp_read(fuse, &val);
		if (res < 0) {
			lib_printf("Fuse reading failed! (%d)\n", res);
			return CMD_EXIT_FAILURE;
		}

		lib_printf("0x%08x\n", val);
	}
	else if (write != 0) {
		res = otp_write(fuse, val);
		if (res < 0) {
			lib_printf("Fuse write failed! (%d)\n", res);
			return CMD_EXIT_FAILURE;
		}
		lib_printf("write %x = %x\n", fuse, val);
	}

	return CMD_EXIT_SUCCESS;
}


__attribute__((constructor)) static void cmd_otpReg(void)
{
	const static cmd_t app_cmd = { .name = "otp", .run = cmd_otp, .info = cmd_otpInfo };
	cmd_reg(&app_cmd);
}
