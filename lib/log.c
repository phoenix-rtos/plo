/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Logger
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "log.h"


struct {
	int mode;
} echo_common;


int log_getEcho(void)
{
	return echo_common.mode;
}


void log_setEcho(int val)
{
	echo_common.mode = !!val;
}
