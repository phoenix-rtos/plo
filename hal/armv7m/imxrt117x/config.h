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

#include "types.h"
#include "imxrt.h"

#define KERNEL_PATH              "phoenix-armv7m7-imxrt117x.elf"

/* Addresses descriptions */
#define SYSPAGE_ADDRESS          0x20200000
#define STACK_SIZE	             5 * 1024
#define PAGE_SIZE                0x200


/* Linker symbols */
extern void _end(void);
extern void _plo_bss(void);


/* Types extensions */

typedef u32 addr_t;
typedef unsigned int size_t;
typedef int ssize_t;

typedef struct {
	/* Empty hal data */
} syspage_hal_t;



#endif
