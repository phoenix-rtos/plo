/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
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

#include "../types.h"


/* User interface */
#define VERSION                  "1.0.1"
#define PLO_WELCOME              "\n-\\- Phoenix-RTOS loader for Zynq-7000, version: " VERSION
#define PLO_DEFAULT_CMD          "syspage"
#define KERNEL_PATH              "phoenix-armv7a9-zynq7000.elf"


/* Addresses descriptions */
#define DISK_IMAGE_BEGIN         0             /* TBD */
#define DISK_IMAGE_SIZE          0             /* TBD */

#define DISK_KERNEL_OFFS         0             /* TBD */
#define SYSPAGE_ADDRESS          0xffff8000    /* Begin of OCRAM's high address */
#define BISTREAM_ADDR            0x00100000

#define PAGE_SIZE                0x1000


/* Linker symbols */
extern void _end(void);
extern void plo_bss(void);


/* PHFS sources  */
#define PDN_NB                   2
#define PDN_COM1                 0
#define PDN_ACM0                 1

#define PHFS_ACM_PORTS_NB        1    /* Number of ports define by CDC driver; min = 1, max = 2               */
#define PHFS_SERIAL_LOADER_ID    0    /* UART ID on which data exchange is established with phoenixd host app */


/* Types extensions */
typedef u32 addr_t;

typedef unsigned int size_t;


#endif
