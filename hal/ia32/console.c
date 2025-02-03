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
#include "console.h"


static struct halconsole *halconsole_common;


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	struct halconsole *curr;
	for (curr = halconsole_common; curr != NULL; curr = curr->next) {
		curr->setHooks(writeHook);
	}
}


void hal_consolePrint(const char *s)
{
	struct halconsole *curr;
	for (curr = halconsole_common; curr != NULL; curr = curr->next) {
		curr->print(s);
	}
}


void hal_consoleInit(void)
{
	struct halconsole *curr;
	for (curr = halconsole_common; curr != NULL; curr = curr->next) {
		curr->init();
	}
}


void hal_consoleRegister(struct halconsole *console)
{
	console->next = halconsole_common;
	halconsole_common = console;
}
