/*
 * Phoenix-RTOS
 *
 * Entrypoint
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "hal.h"

extern void _end(void);
extern void _plo_bss(void);

extern void (*__init_array_start [])(void);
extern void (*__init_array_end [])(void);

extern void (*__fini_array_start [])(void);
extern void (*__fini_array_end [])(void);


extern int main(void);


void _startc(int argc, char **argv, char **env)
{
	size_t i, size;

	/* Clear the .bss section */
	hal_memset(_plo_bss, 0, (addr_t)_end - (addr_t)_plo_bss);

	size = __init_array_end - __init_array_start;
	for (i = 0; i < size; i++)
		(*__init_array_start[i])();

	main();

	size = __fini_array_end - __fini_array_start;
	for (i = size; i > 0; i--)
		(*__fini_array_start[i - 1])();
}
