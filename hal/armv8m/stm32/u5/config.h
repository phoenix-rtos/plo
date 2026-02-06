/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2020, 2021 Phoenix Systems
 * Copyright 2026 Apator Metrix
 * Author: Hubert Buczynski, Aleksander Kaminski, Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __ASSEMBLY__

#include "stm32u5.h"
#include "peripherals.h"
#include "../types.h"

#include <phoenix/arch/armv8m/stm32/syspage.h>
#include <phoenix/syspage.h>

#include "../../cpu.h"
#include "../../mpu.h"

#define PATH_KERNEL "phoenix-armv8m33-stm32u5.elf"

#endif


/* Import platform specific definitions */
#include "ld/armv8m33-stm32u5.ldt"

#endif
