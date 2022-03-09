/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Common Linker Script for ARM targets
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


/* Specify output format */
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

/* Entry point */
ENTRY(_start)

SECTIONS
{
	. = ORIGIN(PLO_IMAGE);

	.init : { KEEP (*(SORT_NONE(.init))) } > PLO_IMAGE

	.text :
	{
		. = ALIGN(4);
		*(SORT(.text.sorted.*))
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.glue_7)      /* glue arm to thumb code */
		*(.glue_7t)     /* glue thumb to arm code */
		*(.eh_frame)
		. = ALIGN(4);
	} > PLO_IMAGE

	.fini : { KEEP (*(SORT_NONE(.fini))) } > PLO_IMAGE

	/* define a global symbol at end of code */
	PROVIDE (__etext = .);
	PROVIDE (_etext = .);
	PROVIDE (etext = .);

	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) } > PLO_IMAGE

	/* put ARM the section containing information for unwinding the stack */
	.ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > PLO_IMAGE
	.ARM.exidx :
	{
		__exidx_start = .;
		*(.ARM.exidx*)     /* needed when -funwind-tables -fexceptions */
		__exidx_end = .;
	} > PLO_IMAGE

	.init_array :
	{
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);
	} > PLO_IMAGE

	.fini_array :
	{
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP (*(.fini_array .dtors))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} > PLO_IMAGE

	.fastram.text.rel : ALIGN(4)
	{
		__ramtext_load = LOADADDR(.fastram.text.rel);
		__ramtext_start = .;
		/* explicit placement of flash (noxip) and frequently used functions in RAM */
		*(.noxip)
		*(.ramfunc)
		. = ALIGN(4);
		__ramtext_end = .;
	} > TCM_TEXT AT > PLO_IMAGE

	.data : ALIGN(4)
	{
		__data_load = LOADADDR(.data);
		__data_start = .;
		*(.data)
		*(.data*)
		. = ALIGN(4);
		__data_end = .;
		_edata = .;
		PROVIDE (edata = .);
	} > DATA AT > PLO_IMAGE

	.bss (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start = .;
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	} > BSS

	_end = .;
	PROVIDE (end = .);

	.heap (NOLOAD) : ALIGN(SIZE_PAGE)
	{
		__heap_base = .;
		. += SIZE_HEAP;
		. = ALIGN(SIZE_PAGE);
		__heap_limit = .;
	} > HEAP

	.stack (NOLOAD) : ALIGN(8)
	{
		__stack_limit = .;
		. += SIZE_STACK;
		. = ALIGN(8);
		__stack_top = .;
	} > STACK

	PROVIDE(_stacksz = __stack_top - __stack_limit);
	PROVIDE(_stack = __stack_top);

	_plo_size = LOADADDR(.data) + SIZEOF(.data) - ORIGIN(PLO_IMAGE);

	.ARM.attributes 0 : { *(.ARM.attributes) }

	/DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}
