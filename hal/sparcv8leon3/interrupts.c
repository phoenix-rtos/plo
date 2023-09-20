/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupt handling
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


#define STR(x)  #x
#define XSTR(x) STR(x)


inline void hal_interruptsEnableAll(void)
{
	/* clang-format off */

	__asm__ volatile(
		"mov 0, %%o0\n\t"
		"ta 0x09\n\t"
		/* TN-0018 Errata */
		"nop\n\t"
		:
		:
		: "%o0", "memory");

	/* clang-format on */
}


inline void hal_interruptsDisableAll(void)
{
	/* clang-format off */

	__asm__ volatile(
		"mov " XSTR(PSR_PIL) ", %%o0\n\t"
		"ta 0x09\n\t"
		/* TN-0018 Errata */
		"nop\n\t"
		:
		:
		: "%o0", "memory");

	/* clang-format on */
}
