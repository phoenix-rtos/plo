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

#include <board_config.h>

#ifdef ZYNQMP_VIRT
#define GIC_BASE_ADDRESS 0x08000000
#else
#define GIC_BASE_ADDRESS 0xf9010000
#endif

/* RAM storage configuration */
#ifdef ZYNQMP_VIRT
#define RAM_ADDR      0x49000000 /* 128 MB */
#else
#define RAM_ADDR      0x08000000 /* 128 MB */
#endif
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
