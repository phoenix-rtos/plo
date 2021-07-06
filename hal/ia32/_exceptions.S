/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Exception stubs
 *
 * Copyright 2012, 2016, 2021 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__


.code32
.section .text


/* Exception handlers */
.extern exceptions_common


/* Exception stub macro (no error code) */
.macro EXCEPTION_NO_ERR name, exc
.globl \name
.type \name, @function
.align 4, 0x90
\name:
	cli
	pushl $0x0
	call _exceptions_init
	leal (%esp), %eax
	pushl %eax
	pushl $\exc
	movl exceptions_common + 4 * \exc, %eax
	call *%eax
	addl $0x8, %esp
	jmp _exceptions_done
.endm


/* Exception stub macro (with error code) */
.macro EXCEPTION_ERR name, exc
.globl \name
.type \name, @function
.align 4, 0x90
\name:
	cli
	call _exceptions_init
	leal (%esp), %eax
	pushl %eax
	pushl $\exc
	movl exceptions_common + 4 * \exc, %eax
	call *%eax
	addl $0x8, %esp
	jmp _exceptions_done
.endm


/* Saves CPU context and reloads segment registers */
_exceptions_init:
	xchgl (%esp), %edx
	movl %edx, -60(%esp)
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
	movl %dr5, %eax
	pushl %eax
	movl %dr4, %eax
	pushl %eax
	movl %dr3, %eax
	pushl %eax
	movl %dr2, %eax
	pushl %eax
	movl %dr1, %eax
	pushl %eax
	movl %dr0, %eax
	pushl %eax
	subl $0x4, %esp

	/* Reload segments */
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	ret


/* Restores CPU context and returns from exception */
_exceptions_done:
	popl %eax
	movl %eax, %dr0
	popl %eax
	movl %eax, %dr1
	popl %eax
	movl %eax, %dr2
	popl %eax
	movl %eax, %dr3
	popl %eax
	movl %eax, %dr4
	popl %eax
	movl %eax, %dr5
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
	addl $0x4, %esp

	iret


/* Exception stubs */
EXCEPTION_NO_ERR _exceptions_exc0, 0
EXCEPTION_NO_ERR _exceptions_exc1, 1
EXCEPTION_NO_ERR _exceptions_exc2, 2
EXCEPTION_NO_ERR _exceptions_exc3, 3
EXCEPTION_NO_ERR _exceptions_exc4, 4
EXCEPTION_NO_ERR _exceptions_exc5, 5
EXCEPTION_NO_ERR _exceptions_exc6, 6
EXCEPTION_NO_ERR _exceptions_exc7, 7
EXCEPTION_ERR _exceptions_exc8, 8
EXCEPTION_NO_ERR _exceptions_exc9, 9
EXCEPTION_ERR _exceptions_exc10, 10
EXCEPTION_ERR _exceptions_exc11, 11
EXCEPTION_ERR _exceptions_exc12, 12
EXCEPTION_ERR _exceptions_exc13, 13
EXCEPTION_ERR _exceptions_exc14, 14
EXCEPTION_NO_ERR _exceptions_exc15, 15
EXCEPTION_NO_ERR _exceptions_exc16, 16
EXCEPTION_ERR _exceptions_exc17, 17
EXCEPTION_NO_ERR _exceptions_exc18, 18
EXCEPTION_NO_ERR _exceptions_exc19, 19
EXCEPTION_NO_ERR _exceptions_exc20, 20
EXCEPTION_NO_ERR _exceptions_exc21, 21
EXCEPTION_NO_ERR _exceptions_exc22, 22
EXCEPTION_NO_ERR _exceptions_exc23, 23
EXCEPTION_NO_ERR _exceptions_exc24, 24
EXCEPTION_NO_ERR _exceptions_exc25, 25
EXCEPTION_NO_ERR _exceptions_exc26, 26
EXCEPTION_NO_ERR _exceptions_exc27, 27
EXCEPTION_NO_ERR _exceptions_exc28, 28
EXCEPTION_NO_ERR _exceptions_exc29, 29
EXCEPTION_ERR _exceptions_exc30, 30
EXCEPTION_NO_ERR _exceptions_exc31, 31
