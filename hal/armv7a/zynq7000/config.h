/*
 * Phoenix-RTOS
 *
 * Operating system loader
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
#define PATH_KERNEL "phoenix-armv7a9-zynq7000.elf"

/* Addresses descriptions */
#define ADDR_SYSPAGE   0xffff8000 /* Begin of OCRAM's high address */
#define ADDR_BITSTREAM 0x00100000

#define SIZE_PAGE 0x1000

#endif
