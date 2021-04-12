/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * System timer driver
 *
 * Copyright 2012, 2020 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include "types.h"

#define TIMER_EXPIRE   0
#define TIMER_KEYB     2
#define TIMER_VALCHG   4


extern int timer_wait(u32 ms, int flags, u16 *p, u16 v);

extern void timer_init(void);

extern void timer_done(void);


#endif
