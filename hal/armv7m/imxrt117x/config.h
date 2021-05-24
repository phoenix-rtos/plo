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
#include "../string.h"


#define KERNEL_PATH "phoenix-armv7m7-imxrt117x.elf"

/* Addresses descriptions */
#define DISK_IMAGE_BEGIN 0x70000000
#define DISK_IMAGE_SIZE  0x0013f000

#define DISK_KERNEL_OFFS 0x00011000
#define SYSPAGE_ADDRESS  0x202c0000

#define STACK_SIZE 5 * 1024
#define PAGE_SIZE  0x200

#endif
