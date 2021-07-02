/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Interrupt stubs
 *
 * Copyright 2012-2013, 2016-2017, 2020, 2021 Phoenix Systems
 * Copyright 2001, 2005-2006 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Jan Sikorski, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__


.code32
.section .text


/* Dispatches interrupts */
.extern interrupts_dispatch
.type interrupts_dispatch, @function


/* Acknowledges interrupts in controller */
.extern interrupts_ack
.type interrupts_ack, @function


/* Interrupt stub macro */
.macro INTERRUPT name, intr, func
.globl \name
.type \name, @function
.align 4, 0x90
\name:
	call _interrupts_init
	pushl $\intr
	call \func
	call interrupts_ack
	addl $0x4, %esp
	jmp _interrupts_done
.endm


/* Saves CPU context and reloads segment registers */
_interrupts_init:
	xchgl (%esp), %edx
	movl %edx, -40(%esp)
	popl %edx
	pushw %ds
	pushw %es
	pushw %fs
	pushw %gs
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %ebp
	pushl %esi
	pushl %edi
	pushl %esp
	subl $0x4, %esp

	/* Reload segments */
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	ret


/* Restores CPU context and returns from interrupt */
_interrupts_done:
	popl %esp
	popl %edi
	popl %esi
	popl %ebp
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax
	popw %gs
	popw %fs
	popw %es
	popw %ds

	iret


/* Interrupt stubs */
INTERRUPT _interrupts_irq0, 0, interrupts_dispatch
INTERRUPT _interrupts_irq1, 1, interrupts_dispatch
INTERRUPT _interrupts_irq2, 2, interrupts_dispatch
INTERRUPT _interrupts_irq3, 3, interrupts_dispatch
INTERRUPT _interrupts_irq4, 4, interrupts_dispatch
INTERRUPT _interrupts_irq5, 5, interrupts_dispatch
INTERRUPT _interrupts_irq6, 6, interrupts_dispatch
INTERRUPT _interrupts_irq7, 7, interrupts_dispatch
INTERRUPT _interrupts_irq8, 8, interrupts_dispatch
INTERRUPT _interrupts_irq9, 9, interrupts_dispatch
INTERRUPT _interrupts_irq10, 10, interrupts_dispatch
INTERRUPT _interrupts_irq11, 11, interrupts_dispatch
INTERRUPT _interrupts_irq12, 12, interrupts_dispatch
INTERRUPT _interrupts_irq13, 13, interrupts_dispatch
INTERRUPT _interrupts_irq14, 14, interrupts_dispatch
INTERRUPT _interrupts_irq15, 15, interrupts_dispatch
INTERRUPT _interrupts_unexpected, 255, _interrupts_unexpected


/* Remaps interrupt controller vector offsets */ 
.extern interrupts_remapPIC
.type interrupts_remapPIC, @function


/* FIXME: temporary HAL syspage, should be replaced by new syspage implementation */
/* Loader syspage */
.extern _plo_syspage


/* Executes BIOS interrupt call */
.globl _interrupts_bios
.type _interrupts_bios, @function
_interrupts_bios:
	cli

	/* Save BIOS interrupt %eax and %flags */
	pushl %eax
	pushf
	popl %eax
	pushw %ax

	/* Store BIOS interrupt number */
	movl 18(%esp), %eax
	movb %al, _interrupts_biosIntr

	/* Remap PIC for real mode */
	pushl %ecx
	pushl %edx
	pushl $0x70
	pushl $0x08
	call interrupts_remapPIC
	addl $0x8, %esp
	popl %edx
	popl %ecx

	/* Load real mode IDTR */
	lidt _interrupts_idtr_real

	/* Switch to 16-bit protected mode */
	ljmp $0x18, $_interrupts_prot16

.code16
_interrupts_prot16:
	/* Disable protected mode */
	movl %cr0, %eax
	andl $~0x1, %eax
	movl %eax, %cr0
	ljmp $0x0, $_interrupts_real

_interrupts_real:
	/* Restore real mode stack segment */
	xorw %ax, %ax
	movw %ax, %ss

	/* Restore BIOS interrupt %ds, %es, %flags and %eax */
	movl 14(%esp), %eax
	movw %ax, %ds
	movl 10(%esp), %eax
	movw %ax, %es
	popf
	popl %eax
	sti

	/* INT imm8 */
	.byte 0xcd
_interrupts_biosIntr:
	.byte 0

	/* Save %eax and %flags */
	cli
	pushl %eax
	pushf

	/* Enable protected mode */
	movl %cr0, %eax
	orl $0x1, %eax
	movl %eax, %cr0
	ljmp $0x8, $_interrupts_prot32

.code32
_interrupts_prot32:
	/* Restore protected mode data segments */
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	/* Remap PIC for protected mode */
	pushl %ecx
	pushl %edx
	pushl $0x28
	pushl $0x20
	call interrupts_remapPIC
	addl $0x8, %esp
	popl %edx
	popl %ecx

	/* FIXME: temporary HAL syspage reference, should be replaced by new syspage implementation reference */
	/* Reload protected mode IDTR */
	lidt _plo_syspage + 0x8

	/* Restore %flags and %eax */
	pushf
	popl %eax
	popw %ax
	pushl %eax
	popf
	popl %eax

	sti
	ret


.section .rodata


/* Real mode IDT descriptor (Interrupt Vector Table) */
_interrupts_idtr_real:
	.word 0x400 - 1
	.long 0
