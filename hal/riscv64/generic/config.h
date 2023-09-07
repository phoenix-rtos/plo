/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifndef __ASSEMBLY__

#include "peripherals.h"
#include "types.h"
#include "hal/riscv64/cpu.h"
#include "hal/riscv64/plic.h"
#include "hal/riscv64/sbi.h"
#include "hal/riscv64/dtb.h"

#include <phoenix/arch/syspage-riscv64.h>
#include <phoenix/syspage.h>

#define PATH_KERNEL "phoenix-riscv64-generic.elf"

#endif /* __ASSEMBLY__ */

/* Import platform specific definitions */
#include "ld/riscv64-generic.ldt"


#endif
