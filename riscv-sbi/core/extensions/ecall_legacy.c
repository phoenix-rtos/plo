/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * SBI IPI handler
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "sbi.h"
#include "devices/console.h"


static sbiret_t ecall_legacy_putcharHandler(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid)
{
	console_putc(a0);

	/* Legacy extensions preserve a1 */
	return (sbiret_t) { .error = SBI_SUCCESS, .value = a1 };
}


static sbiret_t ecall_legacy_getcharHandler(sbi_param a0, sbi_param a1, sbi_param a2, sbi_param a3, sbi_param a4, sbi_param a5, int fid)
{
	long c = console_getc();
	return (sbiret_t) { .error = c, .value = a1 };
}


static const sbi_ext_t sbi_ext_legacy[] __attribute__((section("extensions"), used)) = {
	{ .eid = SBI_EXT_0_1_CONSOLE_PUTCHAR, .handler = ecall_legacy_putcharHandler },
	{ .eid = SBI_EXT_0_1_CONSOLE_GETCHAR, .handler = ecall_legacy_getcharHandler },
};
