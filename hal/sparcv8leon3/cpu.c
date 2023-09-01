/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Leon3 CPU related routines
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


#define BOOTSTRAP_ADDR 0x80008000
#define BOOTSTRAP_SPIM 0x400BC003


void hal_cpuReboot(void)
{
	/* Reset to the built-in bootloader */
	hal_interruptsDisableAll();

	/* Reboot to SPIM */
	*(vu32 *)(BOOTSTRAP_ADDR) = BOOTSTRAP_SPIM;

	/* clang-format off */
	__asm__ volatile (
		"jmp %%g0\n\t"
		"nop\n\t"
		:::
	);
	/* clang-format on */

	__builtin_unreachable();
}
