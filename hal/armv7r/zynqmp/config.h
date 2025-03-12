/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#define GIC_BASE_ADDRESS 0xf9000000

/* RAM storage configuration */
#define RAM_ADDR      0x08000000 /* 128 MB */
#define RAM_BANK_SIZE 0x08000000 /* 128 MB */


#ifndef __ASSEMBLY__

#include "zynqmp.h"
#include "hal/armv7r/types.h"
#include "peripherals.h"

#include <phoenix/arch/armv7r/zynqmp/syspage.h>
#include <phoenix/syspage.h>

#include "../cpu.h"
#include "../mpu.h"

#define PATH_KERNEL "phoenix-armv7r5f-zynqmp.elf"

#endif


/* Import platform specific definitions */
#include "ld/armv7r5f-zynqmp.ldt"

#endif
