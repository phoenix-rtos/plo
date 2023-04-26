/*
 * Phoenix-RTOS
 *
 * Entrypoint
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Hubert Buczynski, Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

extern char __bss_start[], __bss_end[];
extern char __data_load[], __data_start[], __data_end[];
extern char __rodata_load[], __rodata_start[], __rodata_end[];
extern char __ramtext_load[], __ramtext_start[], __ramtext_end[];

extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);


extern int main(void);


void _startc(int argc, char **argv, char **env)
{
	size_t i, size;

	/* Load .fastram.text, .data and .rodata sections */
	if (__ramtext_start != __ramtext_load)
		hal_memcpy(__ramtext_start, __ramtext_load, __ramtext_end - __ramtext_start);

	if (__data_start != __data_load)
		hal_memcpy(__data_start, __data_load, __data_end - __data_start);

	if (__rodata_start != __rodata_load)
		hal_memcpy(__rodata_start, __rodata_load, __rodata_end - __rodata_start);

	/* Clear the .bss section */
	hal_memset(__bss_start, 0, __bss_end - __bss_start);

	size = __init_array_end - __init_array_start;
	for (i = 0; i < size; i++)
		(*__init_array_start[i])();

	main();

	size = __fini_array_end - __fini_array_start;
	for (i = size; i > 0; i--)
		(*__fini_array_start[i - 1])();
}
