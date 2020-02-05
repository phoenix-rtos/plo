/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * A20 line gating
 *
 * Copyright 2020 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "low.h"
#include "a20.h"


u8 a20_test(void)
{
#asm
	push bp
	mov bp, sp
	push ebx
	push ecx
	push esi
	push edi
	push fs

	mov esi, #0x00000900
	mov edi, #0x00100900

	seg fs
	; Read and store the original dword at [edi]
	mov eax, dword ptr [edi]
	; Save the current value
	push eax
	; Assume not set (return 0)
	xor eax, eax
	; Loop 32 times to make sure
	mov ecx, #32

l1:
	add dword ptr [edi], #0x01010101
	mov ebx, dword ptr [edi]
	cmp ebx, dword ptr [esi]
	loope l1

	; If equal jmp over (return 1)
	je word l2
	; al = 1, A20 is set
	inc al

l2:
	; If the values are still the same, the two pointers
	; point to the same location in memory
	; i.e. the A20 line is not set 

	; Restore original value
	pop ebx
	mov dword ptr [edi], ebx

	pop fs
	pop edi
	pop esi
	pop ecx
	pop ebx
	pop bp
#endasm
}


static u8 wait_kbd_status(u8 bit)
{
	u16 i;

	for (i = 0; i < 65535; i++)
		if (!(low_inb(0x64) & (1 << bit)))
			return 1;

	return 0;
}


u8 a20_enable(void)
{
	u8 byte;

	/* A20 already active */
	if (a20_test())
		return 1;

	/* Enable A20 using keyboard controller */
	if (!wait_kbd_status(1))
		return 0;

	low_outb(0x64, 0xd0);

	if (!wait_kbd_status(0))
		return 0;
	
	byte = low_inb(0x60);

	if (!wait_kbd_status(1))
		return 0;

	low_outb(0x64, 0xd1);

	if (!wait_kbd_status(1))
		return 0;

	low_outb(0x60, byte | 2);
	/* Wait for the A20 line to settle down (up to 20usecs) */
	/* Send 0xff (Pulse Output Port NULL) */
	low_outb(0x64, 0xff);

	if (!wait_kbd_status(1))
		return 0;

	/* Now test the A20 line */
	return a20_test();
}
