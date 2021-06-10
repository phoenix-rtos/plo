/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "zynq.h"
#include "types.h"
#include "peripherals.h"
#include "../string.h"


/* User interface */
#define KERNEL_PATH "phoenix-armv7a9-zynq7000.elf"

/* Addresses descriptions */
#define SYSPAGE_ADDRESS 0xffff8000 /* Begin of OCRAM's high address */
#define BITSREAM_ADDR   0x00100000

#define PAGE_SIZE 0x1000

#endif
