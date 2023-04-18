/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Low-level initialization for Cortex-M7 (ARMv7) architecture
 *
 * Copyright 2012, 2016-2017, 2020 Phoenix Systems
 * Author: Jacek Popko, Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

#include "config.h"

.syntax unified
.cpu cortex-m7


#define IOMUXC_GPR 0x400ac000
#define FLEXSPI2_BASE 0x70000000

#define ADDR_ITCM 0x00000000
#define ADDR_DTCM 0x20000000
#define ADDR_SCB  0xe000ed00

.section .init, "x"

.globl _fcfb
.type _fcfb, %object
_fcfb:
.word 0x42464346 /* Tag */
.word 0x56010400 /* Version */
.word 0
.byte 0x1        /* readSampleClkSrc */
.byte 0x3        /* dataHoldTime */
.byte 0x3        /* dataSetupTime */
.byte 0          /* columnAddressWidth */
.byte 0          /* deviceModeCfgEnable */
.byte 0
.byte 0, 0       /* waitTimeCfgCommnads */
.word 0          /* deviceModeSeq */
.word 0          /* deviceModeArg */
.byte 0          /* configCmdEnable */
.byte 0, 0, 0
.word 0, 0, 0, 0 /* configCmdSeqs */
.word 0, 0, 0, 0 /* cfgCmdArgs */
.word 0x00000010 /* controllerMiscOption */
.byte 0x1        /* deviceType */
.byte 0x4        /* sflashPadType */
.byte 0x8        /* serialClkFreq */
.byte 0x0        /* lutCustomSeqEnable */
.word 0, 0
.word 0x800000   /* sflashA1Size */
.word 0          /* sflashA2Size */
.word 0          /* sflashB1Size */
.word 0          /* sflashB2Size */
.word 0          /* csPadSettingOverride */
.word 0          /* sclkPadSettingOverride */
.word 0          /* dataPadSettingOverride */
.word 0          /* dqsPadSettingOverride */
.word 0          /* timeoutInMs */
.word 0          /* commandInterval */
.word 0x00000000 /* dataValidTime */
.byte 0x0, 0     /* busyOffset */
.byte 0x0, 0     /* busyBitPolarity */

/* lookupTable */
.word 0x0818040b, 0x24043008, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000

/* Customizable LUT Sequences */
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000

/* reserved */
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000

.word 0x00000100 /* pageSize */
.word 0x00001000 /* sectorSize */
.word 0x00000001 /* ipCmdSerialClkFreq */
.word 0x00000000

/* reserved */
.word 0x00010000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000


.org _fcfb + 0x1000, 0xff

ivt_flash:
	.word 0x412000d1                         /* hdr */
	.word _start                             /* entry */
	.word 0                                  /* reserved 1 */
	.word 0                                  /* dcd */
	.word flash_boot_data                    /* boot_data */
	.word ivt_flash                          /* self */
	.word 0                                  /* csf */
	.word 0                                  /* reserved 2 */

flash_boot_data:
	.word FLEXSPI2_BASE                      /* load address */
	.word _plo_size                          /* size */
	.word 0                                  /* plugin */
	.word 0xffffffff                         /* empty - extra data word */

ivt_flash_end:

.org _fcfb + 0x1050, 0xff

ivt:
	.word 0x412000d1                         /* hdr */
	.word _start                             /* entry */
	.word 0                                  /* reserved 1 */
	.word 0                                  /* dcd */
	.word boot_data                          /* boot_data */
	.word ivt                                /* self */
	.word 0                                  /* csf */
	.word 0                                  /* reserved 2 */

boot_data:
	.word ADDR_ITCM                          /* load address */
	.word _plo_size                          /* size */
	.word 0                                  /* plugin */
	.word 0xffffffff

ivt_end:

.org _fcfb + 0x1100, 0xff

.globl _init_vectors
.type _init_vectors, %object
_init_vectors:
.word _stack
.word _start

.word _exceptions_dispatch                     /* NMI */
.word _exceptions_dispatch                     /* HardFault */
.word _exceptions_dispatch                     /* MemMgtFault */
.word _exceptions_dispatch                     /* BusFault */
.word _exceptions_dispatch                     /* UsageFault */
.word 0
.word 0
.word 0
.word 0
.word 0                     /* SVC */
.word 0                     /* Debug */
.word 0
.word _interrupts_dispatch  /* PendSV */
.word _interrupts_dispatch  /* Systick */

.rept 217
.word _interrupts_dispatch  /* _interrupts_dispatch */
.endr
.size _init_vectors, .-_init_vectors

.thumb
.thumb_func

.globl _start
.type _start, %function
_start:
	cpsid if

	/* Configure FLEX RAM */
	ldr r8, =IOMUXC_GPR
	ldr r9, =0x00200007
	str r9, [r8, #0x40]

	ldr r9, =FLEXRAM_CONFIG
	str r9, [r8, #0x44]

_plo:
	/* Init vector table and stack pointer */
	ldr r0, =0xe000ed08
	ldr r1, =_init_vectors
	str r1, [r0]
	ldr r0, [r1]
	bic r0, 7
	msr msp, r0
	ldr r8, =_startc
	bx r8

.size _start, .-_start
.ltorg


.globl _interrupts_dispatch
.type _interrupts_dispatch, %function
.section .noxip
_interrupts_dispatch:
	mrs r0, ipsr
	stmdb sp!, {r0, r4-r11, lr}
	bl hal_interruptDispatch
	ldmia sp!, {r0, r4-r11, lr}
	dmb

	bx lr
.size _interrupts_dispatch, .-_interrupts_dispatch
.ltorg


.globl _exceptions_dispatch
.type _exceptions_dispatch, %function
.section .noxip

_exceptions_dispatch:
	cpsid if

	mrs r0, psp
	stmdb sp!, {r0, r4-r11, lr}

	mrs r0, ipsr
	mov r1, sp

	b hal_exceptionsDispatch
.size _exceptions_dispatch, .-_exceptions_dispatch
.ltorg
