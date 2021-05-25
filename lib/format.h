/*
 * Phoenix-RTOS
 *
 * phoenix-rtos loader
 *
 * format - code derived from libphoenix
 *
 * Copyright 2017, 2021 Phoenix Systems
 * Author: Adrian Kepka, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _FORMAT_H_
#define _FORMAT_H_

#include "types.h"


typedef void (*feedfunc)(void *, char);


extern void format_parse(void *ctx, feedfunc feed, const char *format, va_list args);


#endif
