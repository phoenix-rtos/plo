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

#ifndef __ASSEMBLY__

#include "zynqmp.h"
#include "types.h"
#include "peripherals.h"

/* TODO: fix this once the kernel is armv8a */
#include <phoenix/arch/aarch64/zynqmp/syspage.h>
#include <phoenix/syspage.h>

#include "../cpu.h"

#define PATH_KERNEL "phoenix-aarch64a53-zynqmp.elf"

#endif


/* Import platform specific definitions */
#include "ld/aarch64a53-zynqmp.ldt"

#endif
