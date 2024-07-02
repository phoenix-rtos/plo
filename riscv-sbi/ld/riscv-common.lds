/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
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
	. = ORIGIN(SBI_IMAGE);

	.init :
	{
		__init_start = .;
		KEEP (*(.init))
		__init_end = .;
	} > SBI_IMAGE

	.text :
	{
		__text_start = .;
		. = ALIGN(8);
		*(SORT(.text.sorted.*))
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.eh_frame)
		. = ALIGN(8);
		PROVIDE_HIDDEN(__text_end = .);
	} > SBI_IMAGE

	.fini : { KEEP (*(SORT_NONE(.fini))) } > SBI_IMAGE

	/* define a global symbol at end of code */
	PROVIDE (__etext = .);
	PROVIDE (_etext = .);
	PROVIDE (etext = .);

	.rodata :
	{
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		*(.srodata .srodata.*)
	} > SBI_IMAGE

	/* Section dedicated for SBI extensions */
	.extensions : ALIGN(8)
	{
		__ext_start = .;
		KEEP (*(SORT_BY_NAME(extensions)))
		__ext_end = .;
	} > SBI_IMAGE

	/* Section dedicated for UART drivers info */
	.uart_drivers : ALIGN(8)
	{
		__uart_start = .;
		KEEP (*(SORT_BY_NAME(uart_drivers)))
		__uart_end = .;
	} > SBI_IMAGE

	.eh_frame_hdr : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } > SBI_IMAGE
	.eh_frame : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) } > SBI_IMAGE

	.init_array :
	{
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);
	} > SBI_IMAGE

	.fini_array :
	{
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
		KEEP (*(.fini_array .dtors))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} > SBI_IMAGE

	.data : ALIGN(8)
	{
		. = ALIGN(8);
		__data_load = LOADADDR(.data);
		__data_start = .;
		*(.data)
		*(.data*)
		*(.sdata .sdata*)
		. = ALIGN(8);
		PROVIDE( __global_pointer$ = . + 0x800 );
		__data_end = .;
		_edata = .;
		PROVIDE (edata = .);
	} > DATA AT > SBI_IMAGE

	.bss (NOLOAD) :
	{
		. = ALIGN(8);
		__bss_start = .;
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(.sbss .sbss.*)
		*(COMMON)
		. = ALIGN(8);
		__bss_end = .;
	} > BSS

	_end = .;
	PROVIDE (end = .);

	.stack (NOLOAD) : ALIGN(8)
	{
		__stack_limit = .;
		. += SIZE_STACK;
		. = ALIGN(8);
		__stack_top = .;
	} > STACK

	PROVIDE(_stacksz = __stack_top - __stack_limit);
	PROVIDE(_stack = __stack_top);

	_sbi_size = LOADADDR(.data) + SIZEOF(.data) - ORIGIN(SBI_IMAGE);
	_sbi_load_addr = ORIGIN(SBI_IMAGE);

	.payload : ALIGN(0x1000)
	{
		PROVIDE(_payload_start = .);
		*(.payload)
		. = ALIGN(8);
		PROVIDE(_payload_end = .);
	} > PAYLOAD


	/DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}
