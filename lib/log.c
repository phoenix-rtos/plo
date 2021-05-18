/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * logger
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "log.h"


static int echo = 0;


int log_getEcho(void)
{
	return echo;
}


void log_setEcho(int val)
{
	echo = !!val;
}
