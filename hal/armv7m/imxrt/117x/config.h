/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Platform configuration file
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski, Aleksander Kaminski
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
#include "../../mpu.h"
#include "../../string.h"


#define PATH_KERNEL "phoenix-armv7m7-imxrt117x.elf"

/* Addresses descriptions */
#define ADDR_SYSPAGE 0x202c0000

#define SIZE_STACK 5 * 1024
#define SIZE_PAGE  0x200

#endif
