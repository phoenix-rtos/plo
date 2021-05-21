/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * MPCore exception and interrupt handling
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include "cpu.h"

#include <lib/types.h>


/* Function installs new handler for interrupt given by n */
extern int interrupts_setHandler(u16 n, int (*f)(u16, void *), void *data);


/* Function removes handler given by n */
extern int interrupts_deleteHandler(u16 n);


/* Function initializes interrupt handling */
extern void interrupts_init(void);


#endif
