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

#define GIC_BASE_ADDRESS 0xf9010000

/* RAM storage configuration */
#define RAM_ADDR      0x08000000 /* 128 MB */
#define RAM_BANK_SIZE 0x08000000 /* 128 MB */


#ifndef __ASSEMBLY__

#include "zynqmp.h"
#include "types.h"
#include "peripherals.h"

#include <phoenix/arch/aarch64/zynqmp/syspage.h>
#include <phoenix/syspage.h>

#include "../cpu.h"

#define PATH_KERNEL "phoenix-aarch64a53-zynqmp.elf"

#endif


/* Import platform specific definitions */
#include "ld/aarch64a53-zynqmp.ldt"

#endif
