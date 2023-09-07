/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Common Linker Script for RV64 targets
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

OUTPUT_FORMAT("elf64-littleriscv", "elf64-big", "elf64-littleriscv")

OUTPUT_ARCH(riscv)

/* Entry point */
ENTRY(_start)

SECTIONS
{
	. = ORIGIN(PLO_IMAGE);

	.init : {
		__init_start = .;
		KEEP (*(.init))
		__init_end = .;
	} > PLO_IMAGE

	.text :
	{
		__text_start = .;
		. = ALIGN(8);
		*(SORT(.text.sorted.*))
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.eh_frame)
		. = ALIGN(8);
		PROVIDE_HIDDEN(__text_end = .);
	} > PLO_IMAGE

	.fini : { KEEP (*(SORT_NONE(.fini))) } > PLO_IMAGE

	/* define a global symbol at end of code */
	PROVIDE (__etext = .);
	PROVIDE (_etext = .);
	PROVIDE (etext = .);

	.rodata : {
		__rodata_load = LOADADDR(.rodata);
		__rodata_start = .;
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		__rodata_end = .;
	} > PLO_IMAGE

	.eh_frame_hdr : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } > PLO_IMAGE
	.eh_frame : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) } > PLO_IMAGE

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

	.fastram.text.rel : ALIGN(8)
	{
		__ramtext_load = LOADADDR(.fastram.text.rel);
		__ramtext_start = .;
		*(.ramfunc)
		. = ALIGN(8);
		__ramtext_end = .;
	} > PLO_IMAGE

	.data : ALIGN(8)
	{
		. = ALIGN(8);
		__data_load = LOADADDR(.data);
		__data_start = .;
		*(.data)
		*(.data*)
		. = ALIGN(8);
		PROVIDE( __global_pointer$ = . + 0x800 );
		__data_end = .;
		_edata = .;
		PROVIDE (edata = .);
	} > DATA AT > PLO_IMAGE

	.bss (NOLOAD) :
	{
		. = ALIGN(8);
		__bss_start = .;
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(8);
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
	_plo_load_addr = ORIGIN(PLO_IMAGE);

	/DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}
