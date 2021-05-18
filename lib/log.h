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

#ifndef _LOG_PLO_H_
#define _LOG_PLO_H_

#include "log.h"
#include "lib.h"


#define log_info(fmt, ...)  if (log_getEcho()) lib_printf(fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) lib_printf(fmt, ##__VA_ARGS__)


extern int log_getEcho(void);


extern void log_setEcho(int val);


#endif
