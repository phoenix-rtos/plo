/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * Console driver
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_DEVICES_CONSOLE_H_
#define _SBI_DEVICES_CONSOLE_H_


#include "peripherals.h"


typedef struct {
	const char *compatible;
	int (*init)(const char *compatible);
	void (*putc)(char c);
} uart_driver_t;


typedef struct {
	sbi_reg_t reg;
	u32 freq;
	u32 baud;
} uart_info_t;


void console_putc(char c);


void console_print(const char *s);


void console_init(void);


#endif
