/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2022, 2024 Phoenix Systems
 * Author: Damian Loewnau, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __ASSEMBLY__

#include "n94x.h"
#include "peripherals.h"
#include "hal/armv8m/mcx/types.h"

#include <phoenix/arch/armv8m/mcx/syspage.h>
#include <phoenix/syspage.h>

#include "hal/armv8m/cpu.h"
#include "hal/armv8m/mpu.h"

#define PATH_KERNEL "phoenix-armv8m33-mcxn94x.elf"

#endif


/* Import platform specific definitions */
#include "ld/armv8m33-mcxn94x.ldt"

#endif
