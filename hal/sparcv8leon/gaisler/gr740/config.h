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

#include <types.h>
#include "gr740.h"
#include "peripherals.h"
#include "hal/sparcv8leon/gaisler/gaisler.h"
#include "hal/sparcv8leon/cpu.h"

#include <phoenix/arch/sparcv8leon/syspage.h>
#include <phoenix/syspage.h>

#define PATH_KERNEL "phoenix-sparcv8leon-gr740.elf"

#endif /* __ASSEMBLY__ */


#define NWINDOWS 8

/* Import platform specific definitions */
#include "ld/sparcv8leon-gr740.ldt"


#endif
