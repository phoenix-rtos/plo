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

#include "types.h"
#include "imx6ull.h"
#include "../cpu.h"

#include <phoenix/arch/syspage-imx6ull.h>
#include <phoenix/syspage.h>

#define PATH_KERNEL "phoenix-armv7a7-imx6ull.elf"

#define PHFS_ACM_PORTS_NB 1 /* Number of ports define by CDC driver; min = 1, max = 2 */

#endif


/* Import platform specific definitions */
#include "ld/armv7a7-imx6ull.ldt"

#endif
