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

#ifndef __ASSEMBLY__

#include "tda4vm.h"
#include "hal/armv7r/types.h"
#include "peripherals.h"

#include <phoenix/arch/armv7r/tda4vm/syspage.h>
#include <phoenix/syspage.h>

#include "../cpu.h"
#include "../mpu.h"

#define PATH_KERNEL "phoenix-armv7r5f-tda4vm.elf"

#endif

#define RAT_REGION_MSRAM_REMAP   0
#define RAT_REGION_VECTORS_REMAP 15
/* Import platform specific definitions */
#include "ld/armv7r5f-tda4vm.ldt"

#endif
