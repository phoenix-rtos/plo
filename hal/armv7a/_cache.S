/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARMv7 Cortex-A cache management
 *
 * Copyright 2022 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#define __ASSEMBLY__

.arm

.globl hal_dcacheEnable
.type hal_dcacheEnable, %function
hal_dcacheEnable:
	dsb
	mrc p15, 0, r1, c1, c0, 0
	cmp r0, #0
	ite ne
	orrne r1, r1, #(0x1 << 2)
	biceq r1, r1, #(0x1 << 2)
	mcr p15, 0, r1, c1, c0, 0
	dsb
	isb
	bx lr
.size hal_dcacheEnable, .-hal_dcacheEnable
.ltorg


.globl hal_dcacheInvalAll
.type hal_dcacheInvalAll, %function
hal_dcacheInvalAll:
	dsb
	isb
	mrc p15, 1, r0, c0, c0, 0        /* Read CCSIDR (Cache Size Identification Register) */
	mov r3, #0x1ff
	and r0, r3, r0, lsr #13          /* r0 = number of sets -                            */
	mov r1, #0                       /* r1 = way counter way_loop                        */
way_loop:
	mov r3, #0                       /* r3 = set counter set_loop                        */
set_loop:
	mov r2, r1, lsl #30
	orr r2, r3, lsl #5               /* r2 = set/way cache operation format              */
	mcr p15, 0, r2, c7, c6, 2        /* Invalidate line described by r2; write to DCISW  */
	add r3, r3, #1                   /* Increment set counter                            */
	cmp r0, r3                       /* Check whether last set was reached               */
	bgt set_loop                     /* Iterate set_loop                                 */
	add r1, r1, #1                   /* Increment way counter                            */
	cmp r1, #4                       /* Check whether last way was reached               */
	bne way_loop
	dsb
	bx lr
.size hal_dcacheInvalAll, .-hal_dcacheInvalAll
.ltorg



.globl hal_dcacheInval
.type hal_dcacheInval, %function
hal_dcacheInval:
	dsb
	mrc p15, 0, r3, c0, c0, 1 /* Read the Cache Type Register */
	lsr r3, r3, #16           /* DMinLine value */
	and r3, r3, #0xf
	mov r2, #4
	mov r2, r2, lsl r3        /* Cache line size in bytes */
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


.globl hal_dcacheClean
.type hal_dcacheClean, %function
hal_dcacheClean:
	dsb
	mrc p15, 0, r3, c0, c0, 1  /* Read the Cache Type Register */
	lsr r3, r3, #16            /* DMinLine value */
	and r3, r3, #0xf
	mov r2, #4
	mov r2, r2, lsl r3         /* Cache line size in bytes */
	sub r3, r2, #1             /* Cache line size mask */
	bic r0, r0, r3
clean_line:
	mcr p15, 0, r0, c7, c10, 1 /* DCCMVAC */
	add r0, r0, r2
	cmp r0, r1
	blo clean_line
	dsb
	isb
	bx lr
.size hal_dcacheClean, .-hal_dcacheClean
.ltorg


.globl hal_dcacheFlush
.type hal_dcacheFlush, %function
hal_dcacheFlush:
	dsb
	mrc p15, 0, r3, c0, c0, 1  /* Read the Cache Type Register */
	lsr r3, r3, #16            /* DMinLine value */
	and r3, r3, #0xf
	mov r2, #4
	mov r2, r2, lsl r3         /* Cache line size in bytes */
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
	dsb
	mrc p15, 0, r1, c1, c0, 0
	cmp r0, #0
	ite ne
	orrne r1, r1, #(0x1 << 12)
	biceq r1, r1, #(0x1 << 12)
	mcr p15, 0, r1, c1, c0, 0
	dsb
	isb
	bx lr
	bx lr
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