/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Low-level initialization for Cortex-M7 (ARMv7) architecture - plo starting from RAM
 *
 * Copyright 2012, 2016-2017, 2020 Phoenix Systems
 * Author: Jacek Popko, Pawel Pisarczyk, Jakub Sejdak, Aleksander Kaminski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#define __ASSEMBLY__

.syntax unified
.cpu cortex-m7

.section .init, "x"

ivt:
	.word 0x402000d1                         /* hdr */
	.word _start                             /* entry */
	.word 0                                  /* reserved 1 */
	.word 0                                  /* dcd */
	.word boot_data                          /* boot_data */
	.word ivt                                /* self */
	.word 0                                  /* csf */
	.word 0                                  /* reserved 2 */

boot_data:
	.word ivt                                /* load address */
	.word _edata - ivt                       /* size */
	.word 0                                  /* plugin */
	.word 0

ivt_end:

.org ivt + 0x1000, 0x0

#include "_common.inc"
