/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration
 *
 * Copyright 2022, 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
# SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifndef __ASSEMBLY__

#include "../types.h"
#include "gr716.h"
#include "peripherals.h"
#include "../gaisler.h"
#include "../../cpu.h"

#include <phoenix/arch/sparcv8leon/syspage.h>
#include <phoenix/syspage.h>

#include <devices/gpio-grlib/gpio.h>

#define PATH_KERNEL "phoenix-sparcv8leon-gr716.elf"

#endif /* __ASSEMBLY__ */


#define NWINDOWS 31

/* Import platform specific definitions */
#include "ld/sparcv8leon-gr716.ldt"


#endif
