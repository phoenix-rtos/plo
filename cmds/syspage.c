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
	lib_printf("sets syspage address or shows it, usage: syspage <address>");
}


static int cmd_syspage(char *s)
{
	char *end;
	addr_t addr;
	unsigned int argsc;
	char (*args)[SIZE_CMD_ARG_LINE];

	argsc = cmd_getArgs(s, DEFAULT_BLANKS, &args);
	if (argsc == 0) {
		syspage_showAddr();
		return ERR_NONE;
	}
	else if (argsc > 1) {
		log_error("\nWrong args %s", s);
		return ERR_ARG;
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
