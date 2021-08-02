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

#ifndef __ASSEMBLY__

#include "zynq.h"
#include "types.h"
#include "peripherals.h"
#include "../string.h"

#define PATH_KERNEL "phoenix-armv7a9-zynq7000.elf"

#endif


#define SIZE_PAGE    0x1000
#define SIZE_STACK   (4 * SIZE_PAGE)
#define SIZE_SYSPAGE (5 * SIZE_PAGE)

#define ADDR_BITSTREAM 0x00100000
#define ADDR_STACK     (_end + SIZE_SYSPAGE + SIZE_STACK)

#endif
