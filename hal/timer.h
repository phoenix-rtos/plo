/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Timer driver
 *
 * Copyright 2012, 2020-2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <lib/types.h>

#define TIMER_EXPIRE 0
#define TIMER_VALCHG 4


/* Function initializes timer controller */
extern void timer_init(void);


/* Function waits for specific period of time or event */
extern int timer_wait(u32 ms, int flags, volatile u16 *p, u16 v);


/* Function resets timer controller */
extern void timer_done(void);


#endif
