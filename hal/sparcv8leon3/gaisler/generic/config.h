/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifndef __ASSEMBLY__

#include "generic.h"
#include "peripherals.h"
#include "hal/sparcv8leon3/gaisler/gaisler.h"
#include "hal/sparcv8leon3/gaisler/types.h"
#include "hal/sparcv8leon3/cpu.h"

#include <phoenix/arch/sparcv8leon3/syspage.h>
#include <phoenix/syspage.h>

#define PATH_KERNEL "phoenix-sparcv8leon3-generic.elf"

#endif /* __ASSEMBLY__ */


#define NWINDOWS 8

/* Import platform specific definitions */
#include "ld/sparcv8leon3-generic.ldt"


#endif
