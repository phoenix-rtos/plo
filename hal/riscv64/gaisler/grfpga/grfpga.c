/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRFPGA CPU specific functions
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "grfpga.h"


extern void console_init(void);


void cpu_init(void)
{
	console_init();
}
