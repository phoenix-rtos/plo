/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * A20 line gating
 *
 * Copyright 2020 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _A20_H_
#define _A20_H_

#include "types.h"


extern u8 a20_test(void);

extern u8 a20_enable(void);


#endif
