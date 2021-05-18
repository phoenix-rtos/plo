/*
 * Phoenix-RTOS
 *
 * plo - perating system loader
 *
 * syspage command
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "hal.h"
#include "cmd.h"
#include "syspage.h"


static void cmd_syspageInfo(void)
{
	lib_printf("shows syspage contents, usage: syspage");
}


static int cmd_syspage(char *s)
{
	char *end;
	addr_t addr;
	unsigned int pos = 0, argsc;
	char args[2][SIZE_CMD_ARG_LINE + 1];

	for (argsc = 0; argsc < 2; ++argsc) {
		if (cmd_getnext(s, &pos, DEFAULT_BLANKS, NULL, args[argsc], sizeof(args[argsc])) == NULL || *args[argsc] == 0)
			break;
	}

	if (argsc > 1) {
		log_error("\nWrong args %s", s);
		return ERR_ARG;
	}

	if (argsc == 0) {
		syspage_showAddr();
		return ERR_NONE;
	}

	addr = lib_strtoul(args[0], &end, 0);
	if (hal_strlen(end) != 0) {
		log_error("\nWrong args %s", s);
		return ERR_ARG;
	}

	syspage_setAddress((void *)addr);
	log_info("\nSetting address: 0x%x", addr);

	return ERR_NONE;
}


__attribute__((constructor)) static void cmd_syspageReg(void)
{
	const static cmd_t app_cmd = { .name = "syspage", .run = cmd_syspage, .info = cmd_syspageInfo };

	cmd_reg(&app_cmd);
}
