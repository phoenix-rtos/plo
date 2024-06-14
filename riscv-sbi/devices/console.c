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


#include "devices/console.h"


extern const uart_driver_t __uart_start[], __uart_end[];


static struct {
	const uart_driver_t *drv;
} console_common;


void console_putc(char c)
{
	if (console_common.drv != NULL) {
		console_common.drv->putc(c);
	}
}


void console_print(const char *s)
{
	while (*s != '\0') {
		console_putc(*s++);
	}
}


void console_init(void)
{
	const uart_driver_t *drv;

	for (drv = __uart_start; drv < __uart_end; drv++) {
		if (drv->init(drv->compatible) == 0) {
			console_common.drv = drv;
			break;
		}
	}
}
