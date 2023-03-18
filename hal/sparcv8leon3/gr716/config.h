/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifndef __ASSEMBLY__

#include "gr716.h"
#include "types.h"
#include "peripherals.h"
#include "../cpu.h"

#include <phoenix/arch/syspage-sparcv8leon3.h>
#include <phoenix/syspage.h>

#include <devices/gpio-gr716/gpio.h>

#define PATH_KERNEL "phoenix-sparcv8leon3-gr716.elf"

#endif /* __ASSEMBLY__ */


#define NWINDOWS    31
#define SYSCLK_FREQ 50000000 /* Hz */

/* Import platform specific definitions */
#include "ld/sparcv8leon3-gr716.ldt"


#endif
