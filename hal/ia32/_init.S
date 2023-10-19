/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low level initialization
 *
 * Copyright 2001, 2005, 2006 Pawel Pisarczyk
 * Copyright 2012, 2020, 2021 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "config.h"


.code16
.section .init


/* External variables */
.extern _plo_size
.extern _plo_seg16
.extern _stack
.extern _stacksz
.extern _startc
.type _startc, @function


/* Loader entry point */
.globl _start
_start:
	/* Reload code segment */
	ljmp $0x0, $_plo_start

_plo_start:
	cli

	/* Set real mode data segments and stack */
	xorw %ax, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $_stack, %sp

	sti

	/* Store boot disk number */
	movb %dl, _plo_bdn

_plo_lba:
	/* Check disk LBA support */
	movw $0x55aa, %bx
	movb $0x41, %ah
	int $0x13
	jc _plo_chs
	cmpw $0xaa55, %bx
	jne _plo_chs
	testb $0x1, %cl
	jz _plo_chs

	/* Set start LBA address */
	xorl %eax, %eax

	/* Set read disk vector */
	movw $_plo_load_lba, _plo_load_sec
	jmp _plo_load

_plo_chs:
	/* Read disk parameters */
	xorw %di, %di
	movb _plo_bdn, %dl
	movb $0x8, %ah
	int $0x13
	jc _plo_end

	/* Store heads and sectors */
	movw %dx, %si
	shrw $0x8, %si
	movw %cx, %di
	andw $0x3f, %di

	/* Set start CHS address */
	xorw %bp, %bp
	xorb %dh, %dh
	movw $0x1, %ax

_plo_load:
	/* Load high level part into memory */
	movw $(_plo_seg16), %bx
	movw %bx, %es
	xorw %bx, %bx
	movb _plo_bdn, %dl
	movl $_plo_size - 1, %ecx
	shrl $0x9, %ecx

_plo_load1:
	/* Increment sector */
	incl %eax

	/* Read sector from disk */
	call *_plo_load_sec
	jc _plo_end

	/* Move offset */
	addw $0x200, %bx
	jnc _plo_load2

	/* Move segment */
	movw %es, %bx
	addw $0x1000, %bx
	movw %bx, %es
	xorw %bx, %bx

_plo_load2:
	loop _plo_load1

	/* Setup GDT */
	/* 32-bit code segment descriptor (selector 0x8) */
	movw $0xffff, (ADDR_GDT + 0x8)
	movw $0x0000, (ADDR_GDT + 0xa)
	movw $0x9a00, (ADDR_GDT + 0xc)
	movw $0x00cf, (ADDR_GDT + 0xe)

	/* 32-bit data segment descriptor (selector 0x10) */
	movw $0xffff, (ADDR_GDT + 0x10)
	movw $0x0000, (ADDR_GDT + 0x12)
	movw $0x9200, (ADDR_GDT + 0x14)
	movw $0x00cf, (ADDR_GDT + 0x16)

	/* 16-bit code segment descriptor (selector 0x18) */
	movw $0xffff, (ADDR_GDT + 0x18)
	movw $0x0000, (ADDR_GDT + 0x1a)
	movw $0x9a00, (ADDR_GDT + 0x1c)
	movw $0x0000, (ADDR_GDT + 0x1e)

	cli

	/* Load IDTR and GDTR */
	lidt _plo_syspage_idtr
	lgdt _plo_syspage_gdtr

	/* Enable protected mode */
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0
	ljmp $0x8, $_plo_prot

.code32
_plo_prot:
	/* Setup protected mode data segments and stack */
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	movl $_stack, %esp

	/* Call high level part */
	call _startc

	/* Reboot in protected mode (triple fault) */
	lidt _plo_idtr_empty
	int3

.code16
_plo_end:
	/* Reboot in real mode (reset vector) */
	ljmp $0xffff, $0x0


/* CHS read disk sector */
_plo_load_chs:
	/* Check track limit */
	cmpw %di, %ax
	jna _plo_load_chs1

	/* Sector exceeded */
	incb %dh
	movw %si, %ax
	cmpb %al, %dh
	movw $0x1, %ax
	jna _plo_load_chs1

	/* Head exceeded */
	incw %bp
	xorb %dh, %dh

_plo_load_chs1:
	pushw %cx
	movw %bp, %cx
	xchgb %cl, %ch
	rorb $0x2, %cl
	orb %al, %cl
	pushw %ax
	movw $0x201, %ax
	int $0x13
	popw %ax
	popw %cx
	ret


/* LBA read disk sector */
_plo_load_lba:
	movw %sp, %bp
	pushl $0x0
	pushl %eax
	pushw %es
	pushw %bx
	pushl $0x10010
	movw %sp, %si
	movb $0x42, %ah
	int $0x13
	movl -8(%bp), %eax
	movw %bp, %sp
	ret


/* Vector for reading disk sector */
_plo_load_sec:
	.word _plo_load_chs


/* Boot disk number */
.globl _plo_bdn
_plo_bdn:
	.byte 0


/* FIXME: temporary HAL syspage, should be replaced by new syspage implementation */
/* Loader syspage */
.globl _plo_syspage
_plo_syspage:
/* GDT descriptor */
_plo_syspage_gdtr:
	.word SIZE_GDT - 1
	.long ADDR_GDT
	.word 0
/* IDT descriptor */
_plo_syspage_idtr:
	.word SIZE_IDT - 1
	.long ADDR_IDT
	.word 0
/* Kernel page directory */
_plo_syspage_pdir:
	.long ADDR_PDIR
/* Kernel page table */
_plo_syspage_ptable:
	.long ADDR_PTABLE
/* Kernel stack */
_plo_syspage_stack:
	.long _stack
/* Kernel stack size */
_plo_syspage_stacksz:
	.long _stacksz


/* Empty IDT descriptor */
.globl _plo_idtr_empty
_plo_idtr_empty:
	.word 0
	.long 0


/* Pad boot sector to 446 bytes (end of bootstrap code area) */
.fill 0x1be - (. - _start), 1, 0


/* MBR partition table */
.fill 0x40, 1, 0


/* MBR signature */
.word 0xaa55
