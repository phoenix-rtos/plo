/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2020, 2021 Phoenix Systems
 * Author: Hubert Buczynski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __ASSEMBLY__

#include "stm32l4.h"
#include "peripherals.h"
#include "../types.h"

#include <phoenix/arch/syspage-stm32.h>
#include <phoenix/syspage.h>

#include "../../cpu.h"
#include "../../mpu.h"

#define PATH_KERNEL "phoenix-armv7m4-stm32l4x6.elf"

#endif


/* Import platform specific definitions */
#include "ld/armv7m4-stm32l4x6.ldt"

#endif
