/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * comamnds script parser
 *
 * Copyright 2020 Phoenix Systems
 * Authors: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "script.h"
#include "plostd.h"
#include "cmd.h"
#include "hal.h"


static char *script[] = {
#include <script.plo.h>
NULL
};


void script_init(void)
{
}


void script_run(void)
{
	unsigned int i;

	for (i = 0; script[i] != NULL; i++) {
		if (script[i][0] != '@') {
			plostd_printf(ATTR_INIT, "\n%s", script[i]);
			cmd_parse(script[i]);
		}
	}
}


int script_expandAlias(char **name)
{
	unsigned int i, len, pos;

	/* Computing the length of the name and positions of the arguments */
	len = plostd_strlen(*name);
	if (len == 1)
		return -1;

	pos = 1;
	while (pos < len) {
		if ((*name)[pos] == ';')
			break;
		++pos;
	}
	--pos;

        for (i = 0; script[i] != NULL; ++i) {
		if (script[i][0] == '@') {
			if (plostd_strncmp(*name + 1, script[i] + 1, len) == 0) {
				/* Copy size and offset to app name */
				hal_memcpy(*name + len, script[i] + pos + 1, plostd_strlen(script[i]) - pos);
				return 0;
			}
		}
	}

	return -1;
}
