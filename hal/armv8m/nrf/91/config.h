/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __ASSEMBLY__

#include "nrf91.h"
#include "peripherals.h"
#include "../types.h"

#include <phoenix/arch/syspage-nrf91.h>
#include <phoenix/syspage.h>

#include "../../cpu.h"

#define PATH_KERNEL "phoenix-armv8m33-nrf9160.elf"

#endif


/* Import platform specific definitions */
#include "ld/armv8m33-nrf9160.ldt"

#endif
