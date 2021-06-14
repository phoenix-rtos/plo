/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "imxrt.h"
#include "peripherals.h"
#include "../types.h"
#include "../../string.h"


/* User interface */
#define KERNEL_PATH "phoenix-armv7m7-imxrt106x.elf"

/* Addresses descriptions */
#define SYSPAGE_ADDRESS 0x20200000
#define STACK_SIZE      5 * 1024
#define PAGE_SIZE       0x200


#endif
