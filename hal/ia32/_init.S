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


/* Loader high level part entry point */
.extern _startc
.type _startc, @function


/* Loader size */
.extern _plo_size


/* Loader entry point */
.globl _start
_start:
	/* Reload code segment */
	ljmp $0x0, $_plo_start

_plo_start:
	cli

	/* Setup real mode data segments and stack */
	xorw %ax, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $_stack, %sp

	/* Store boot disk number */
	movb %dl, _plo_bdn

	sti

	/* Extended read disk parameters */
	movw $_plo_geo, %si
	movb $0x48, %ah
	int $0x13
	jnc _plo_load

	/* Fallback to CHS read disk parameters */
	xorw %di, %di
	movb $0x8, %ah
	int $0x13
	jc _plo_end

	/* Store cylinders */
	xorl %eax, %eax
	movb %ch, %al
	movb %cl, %ah
	andb $0xc0, %ah
	rolb $0x2, %ah
	incl %eax
	movl %eax, _plo_geo_cyls

	/* Store heads */
	xorl %eax, %eax
	movb %dh, %al
	incl %eax
	movl %eax, _plo_geo_heads

	/* Store sectors */
	xorl %eax, %eax
	movb %cl, %al
	andb $0x3f, %al
	movl %eax, _plo_geo_secs

_plo_load:
	/* Load high level part into memory */
	movl $_plo_size-1, %ecx
	shrl $0x9, %ecx

_plo_load1:
	/* Extended read sectors from disk */
	movw $_plo_dap, %si
	movb $0x42, %ah
	int $0x13
	jnc _plo_load2

	/* Fallback to CHS read sectors from disk */
	pushw %cx
	movb _plo_chs_head, %dh
	movw _plo_chs_cyl, %cx
	xchgb %cl, %ch
	rorb $0x2, %cl
	movb _plo_chs_sec, %al
	orb %al, %cl
	movw _plo_dap_seg, %es
	movw _plo_dap_offs, %bx
	movw $0x201, %ax
	int $0x13
	popw %cx
	jc _plo_end

_plo_load2:
	/* Increment sector */
	incl _plo_dap_sec
	incb _plo_chs_sec

	/* Check track limit */
	movb _plo_geo_secs, %al
	cmp %al, _plo_chs_sec
	jna _plo_load3

	/* Sector exceeded */
	movb $0x1, _plo_chs_sec
	incb _plo_chs_head
	movl _plo_geo_heads, %eax
	decl %eax
	cmpb %al, _plo_chs_head
	jna _plo_load3

	/* Head exceeded */
	movb $0x0, _plo_chs_head
	incw _plo_chs_cyl

_plo_load3:
	/* Move offset */
	addw $0x200, _plo_dap_offs
	jnc _plo_load4

	/* Increment segment */
	addw $0x1000, _plo_dap_seg
	movw $0x0, _plo_dap_offs

_plo_load4:
	loop _plo_load1

	/* Setup GDT */
	cli

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


/* Disk geometry packet */
_plo_geo:
/* Packet length */
_plo_geo_len:
	.word 0x1e
/* Flags */
_plo_geo_flags:
	.word 0
/* Cylinders */
_plo_geo_cyls:
	.long 0
/* Heads */
_plo_geo_heads:
	.long 0
/* Sectors per track */
_plo_geo_secs:
	.long 0
/* Total sectors */
_plo_geo_size:
	.quad 0
/* Sector size */
_plo_geo_secsz:
	.word 0x200
/* EDD pointer */
_plo_geo_edd:
	.long 0


/* Disk address packet */
_plo_dap:
/* Packet length */
_plo_dap_len:
	.byte 0x10
/* Unused byte */
	.byte 0
/* Sectors */
_plo_dap_secs:
	.word 0x1
/* Offset */
_plo_dap_offs:
	.word 0
/* Segment */
_plo_dap_seg:
	.word (ADDR_PLO + 0x200) >> 4
/* Sector (LBA) */
_plo_dap_sec:
	.quad 0x1


/* Disk CHS address */
_plo_chs:
/* Cylinder */
_plo_chs_cyl:
	.word 0
/* Head */
_plo_chs_head:
	.byte 0
/* Sector */
_plo_chs_sec:
	.byte 0x2


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
