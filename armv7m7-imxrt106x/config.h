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

#include "../types.h"


/* User interface */

#define VERSION                  "1.0.1"
#define PLO_WELCOME              "\n-\\- Phoenix-RTOS loader for i. MX RT106x, version: " VERSION
#define PLO_DEFAULT_CMD          "syspage"


/* Addresses descriptions */
#define DISK_IMAGE_BEGIN         0x70000000
#define DISK_IMAGE_SIZE          0x0013f000

#define DISK_KERNEL_OFFS         0x00008000
#define SYSPAGE_ADDRESS          0x20200400

#define STACK_SIZE	             5 * 1024


/* Linker symbols */
extern void _end(void);
extern void __bss_start__(void);


/* PHFS sources  */
#define PDN_NB                   3

#define PDN_FLASH0               0
#define PDN_FLASH1               1
#define PDN_COM1                 2

#define PHFS_SERIAL_LOADER_ID    3


/* Types extensions */

typedef u32 addr_t;



#endif
