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

#include "../types.h"


/* User interface */

#define VERSION                  "1.0.1"
#define PLO_WELCOME              "\n-\\- Phoenix-RTOS loader for i. MX RT117x, version: " VERSION
#define PLO_DEFAULT_CMD          "syspage"
#define KERNEL_PATH              "phoenix-armv7m7-imxrt117x.elf"


/* Addresses descriptions */
#define DISK_IMAGE_BEGIN         0x70000000
#define DISK_IMAGE_SIZE          0x0013f000

#define DISK_KERNEL_OFFS         0x00011000
#define SYSPAGE_ADDRESS          0x20200000

#define STACK_SIZE	             5 * 1024

#define PAGE_SIZE                0x200


/* Linker symbols */
extern void _end(void);
extern void plo_bss(void);


/* PHFS sources  */
#define PDN_NB                   2

#define PDN_FLASH0               0
#define PDN_COM1                 1

#define PHFS_SERIAL_LOADER_ID    12
#define PHFS_ACM_PORTS_NB        1    /* Number of ports define by CDC driver; min = 1, max = 2 */


/* Types extensions */

typedef u32 addr_t;
typedef unsigned int size_t;

typedef struct {
	/* Empty architecture data */
} syspage_arch_t;



#endif
