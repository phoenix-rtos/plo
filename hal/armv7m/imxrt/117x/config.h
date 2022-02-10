/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __ASSEMBLY__

#include "imxrt.h"
#include "peripherals.h"

#include "../types.h"

#include <phoenix/arch/syspage-imxrt.h>
#include <phoenix/syspage.h>

#include "../../cpu.h"
#include "../../mpu.h"


#endif


#define PATH_KERNEL "phoenix-armv7m7-imxrt117x.elf"
#define CPU_INFO    "Cortex-M i.MX RT117x"

#define SIZE_PAGE    0x200
#define SIZE_STACK   (8 * SIZE_PAGE)
#define SIZE_SYSPAGE (8 * SIZE_PAGE)

#define ADDR_STACK (_end + SIZE_SYSPAGE + SIZE_STACK)

#endif
