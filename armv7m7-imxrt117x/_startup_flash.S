/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low-level initialization for Cortex-M7 (ARMv7) architecture - plo starting from NV memory
 *
 * Copyright 2012, 2016-2017, 2020 Phoenix Systems
 * Author: Jacek Popko, Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define FLEXSPI2_BASE 0x70000000

#define __ASSEMBLY__

.syntax unified
.cpu cortex-m7

.section .init, "x"
_fcfb:
.word 0x42464346 /* Tag */
.word 0x56010400 /* Version */
.word 0
.byte 0x1        /* readSampleClkSrc */
.byte 0x3        /* dataHoldTime */
.byte 0x3        /* dataSetupTime */
.byte 0          /* columnAdressWidth */
.byte 0          /* deviceModeCfgEnable */
.byte 0
.byte 0, 0       /* waitTimeCfgCommnads */
.word 0          /* deviceModeSeq */
.word 0          /* deviceModeArg */
.byte 0          /* configCmdEnable */
.byte 0, 0, 0
.word 0, 0, 0, 0 /* configCmdSeqs */
.word 0, 0, 0, 0 /* cfgCmdArgs */
.word 0x00000000 /* controllerMiscOption */
.byte 0x0        /* deviceType */
.byte 0x4        /* sflashPadType */
.byte 0x6        /* serialClkFreq */
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
.word 0xa1804eb, 0x26043206, 0x00000000, 0x00000000
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

/* lutCustomSeq */

.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000
.word 0x00000000, 0x00000000, 0x00000000, 0x00000000

.word 0x00000100 /* pageSize */
.word 0x00001000 /* sectorSize */
.word 0x00000000 /* ipCmdSerialClkFreq */
.word 0x00000000
.word 0x00040000

.org _fcfb + 0x1000, 0x0

ivt_flash:
	.word 0x402000d1                         /* hdr */
	.word _start                             /* entry */
	.word 0                                  /* reserved 1 */
	.word 0                                  /* dcd */
	.word flash_boot_data                    /* boot_data */
	.word ivt_flash                          /* self */
	.word 0                                  /* csf */
	.word 0                                  /* reserved 2 */

flash_boot_data:
	.word FLEXSPI2_BASE                      /* load address */
	.word _etext - _fcfb                     /* size */
	.word 0                                  /* plugin */
	.word 0xffffffff                         /* empty - extra data word */

ivt_end:

.org _fcfb + 0x1050, 0x0

#include "_common.inc"
