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
#include "../string.h"
#include "../cpu.h"

#define PATH_KERNEL "phoenix-armv7a7-imx6ull.elf"

#define PHFS_ACM_PORTS_NB 1 /* Number of ports define by CDC driver; min = 1, max = 2 */

#endif

#define SIZE_PAGE    0x1000
#define SIZE_SYSPAGE (2 * SIZE_PAGE)

#define ADDR_DDR 0x80000000
#define SIZE_DDR (128 * 1024 * 1024)

#define ADDR_STACK (_end + SIZE_SYSPAGE + SIZE_STACK)
#define SIZE_STACK (2 * SIZE_PAGE)

#define ADDR_OCRAM 0x907000
#define SIZE_OCRAM 0x020000

#endif
