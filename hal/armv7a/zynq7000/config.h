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

#define PATH_KERNEL "phoenix-armv7a9-zynq7000.elf"

#endif


/* Import platform specific definitions */
#include "ld/armv7a9-zynq7000.ldt"

#endif
