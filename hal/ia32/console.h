/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * HAL console
 *
 * Copyright 2025 Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>


struct halconsole {
	void (*init)(void);
	void (*print)(const char *);
	void (*setHooks)(ssize_t (*writeHook)(int, const void *, size_t));
	struct halconsole *next;
};


void hal_consoleRegister(struct halconsole *console);
