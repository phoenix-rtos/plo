/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR765 CPU specific functions
 *
 * Copyright 2025 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "gr765.h"


extern void console_init(void);


void cpu_init(void)
{
	console_init();
}
