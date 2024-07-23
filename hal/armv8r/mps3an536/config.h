/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __ASSEMBLY__

#include "../types.h"
#include "../cpu.h"

#include <phoenix/arch/armv8r/mps3an536/syspage.h>
#include <phoenix/syspage.h>

#include <board_config.h>


#define PATH_KERNEL "phoenix-armv8r52-mps3an536.elf"

#endif


/* Import platform specific definitions */
#include "ld/armv8r52-mps3an536.ldt"

#endif
