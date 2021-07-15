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

#ifndef _LIB_LOG_H_
#define _LIB_LOG_H_

#include "lib.h"


extern int log_getEcho(void);


extern void log_setEcho(int val);


#define log_info(fmt, ...) \
	do { \
		if (log_getEcho()) \
			lib_printf(fmt, ##__VA_ARGS__); \
	} while (0)


#define log_error(fmt, ...) \
	do { \
		lib_printf(CONSOLE_RED fmt CONSOLE_NORMAL, ##__VA_ARGS__); \
	} while (0)


#endif
