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

#include <phoenix/arch/syspage-zynq7000.h>
#include <phoenix/syspage.h>

#include "../cpu.h"
#include "../string.h"

#define PATH_KERNEL "phoenix-armv7a9-zynq7000.elf"

#endif

#define SIZE_PAGE    0x1000
#define SIZE_SYSPAGE (5 * SIZE_PAGE)

#define ADDR_BITSTREAM 0x00100000
#define ADDR_STACK     (_end + SIZE_SYSPAGE + SIZE_STACK)
#define SIZE_STACK     (4 * SIZE_PAGE)

#define ADDR_OCRAM_LOW  0x00000000
#define SIZE_OCRAM_LOW  (192 * 1024)
#define ADDR_OCRAM_HIGH 0xffff0000
#define SIZE_OCRAM_HIGH (64 * 1024)

#define ADDR_DDR 0x00100000
#define SIZE_DDR (512 * 1024 * 1024)

#endif
