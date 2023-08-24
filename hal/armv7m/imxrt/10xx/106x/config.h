/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
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

#include "../../types.h"

#include <phoenix/arch/syspage-imxrt.h>
#include <phoenix/syspage.h>

#include "../../../cpu.h"
#include "../../../mpu.h"

#define PATH_KERNEL "phoenix-armv7m7-imxrt106x.elf"
#define CPU_INFO    "Cortex-M i.MX RT106x"

#endif


/* Import platform specific definitions */
#include "ld/armv7m7-imxrt106x.ldt"

#endif
