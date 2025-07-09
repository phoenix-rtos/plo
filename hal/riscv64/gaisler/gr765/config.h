/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Platform configuration
 *
 * Copyright 2025 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifndef __ASSEMBLY__

#include "types.h"
#include "hal/riscv64/cpu.h"
#include "hal/riscv64/plic.h"
#include "hal/riscv64/sbi.h"
#include "hal/riscv64/dtb.h"

#include <board_config.h>

#include <phoenix/arch/riscv64/syspage.h>
#include <phoenix/syspage.h>

#define PATH_KERNEL "phoenix-riscv64-gr765.elf"
#define CPU_INFO    "RISC-V 64-bit GR765"


#endif /* __ASSEMBLY__ */

#include "gr765.h"

/* Import platform specific definitions */
#include "ld/riscv64-gr765.ldt"


#endif
