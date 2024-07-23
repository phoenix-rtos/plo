/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv8 Cortex-R cache management
 *
 * Copyright 2022, 2024 Phoenix Systems
 * Author: Hubert Buczynski, Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#define __ASSEMBLY__

.arm

.type cacheEnable, %function
cacheEnable:
	dsb
	mrc p15, 0, r2, c1, c0, 0
	cmp r0, #0
	orrne r2, r2, r1
	biceq r2, r2, r1
	mcr p15, 0, r2, c1, c0, 0
	dsb
	isb
	bx lr
.size cacheEnable, .-cacheEnable
.ltorg


.globl hal_dcacheEnable
.type hal_dcacheEnable, %function
hal_dcacheEnable:
	mov r1, #(1 << 2)
	b cacheEnable
.size hal_dcacheEnable, .-hal_dcacheEnable
.ltorg


.globl hal_dcacheInval
.type hal_dcacheInval, %function
hal_dcacheInval:
	dsb
	mrc p15, 0, r3, c0, c0, 1 /* Read the Cache Type Register */
	ubfx r3, r3, #16, #4      /* DMinLine value */
	mov r2, #4
	lsl r2, r2, r3            /* Cache line size in bytes */
	sub r3, r2, #1            /* Cache line size mask */
	bic r0, r0, r3
inval_line:
	mcr p15, 0, r0, c7, c6, 1 /* DCIMVAC */
	add r0, r0, r2
	cmp r0, r1
	blo inval_line
	dsb
	isb
	bx lr
.size hal_dcacheInval, .-hal_dcacheInval
.ltorg


.globl hal_dcacheFlush
.type hal_dcacheFlush, %function
hal_dcacheFlush:
	dsb
	mrc p15, 0, r3, c0, c0, 1  /* Read the Cache Type Register */
	ubfx r3, r3, #16, #4       /* DMinLine value */
	mov r2, #4
	lsl r2, r2, r3             /* Cache line size in bytes */
	sub r3, r2, #1             /* Cache line size mask */
	bic r0, r0, r3
flush_line:
	mcr p15, 0, r0, c7, c14, 1 /* DCCIMVAC */
	add r0, r0, r2
	cmp r0, r1
	blo flush_line
	dsb
	isb
	bx lr
.size hal_dcacheFlush, .-hal_dcacheFlush
.ltorg


.globl hal_icacheEnable
.type hal_icacheEnable, %function
hal_icacheEnable:
	mov r1, #(1 << 12)
	b cacheEnable
.size hal_icacheEnable, .-hal_icacheEnable
.ltorg


.globl hal_icacheInval
.type hal_icacheInval, %function
hal_icacheInval:
	dsb
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0 /* ICIALLU */
	dsb
	isb
	bx lr
.size hal_icacheInval, .-hal_icacheInval
.ltorg
