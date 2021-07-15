/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Set syspage address
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
#include <syspage.h>


static void cmd_syspageInfo(void)
{
	lib_printf("sets syspage address or shows it, usage: syspage [address]");
}


static int cmd_syspage(int argc, char *argv[])
{
	char *end;
	addr_t addr;

	if (argc == 1) {
		syspage_showAddr();
		return EOK;
	}
	else if (argc > 2) {
		log_error("\n%s: Wrong argument count", argv[0]);
		return -EINVAL;
	}

	addr = lib_strtoul(argv[1], &end, 0);
	if (*end) {
		log_error("\n%s: Wrong arguments", argv[0]);
		return -EINVAL;
	}

	syspage_setAddress(addr);
	log_info("\nSetting address: 0x%x", addr);

	return EOK;
}


__attribute__((constructor)) static void cmd_syspageReg(void)
{
	const static cmd_t app_cmd = { .name = "syspage", .run = cmd_syspage, .info = cmd_syspageInfo };

	cmd_reg(&app_cmd);
}
