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

#define VERSION      "1.0.0"


/* User interface */

#define PLO_WELCOME              "\n-\\- Phoenix-RTOS loader for ia32, version: " VERSION
#define PLO_DEFAULT_CMD          "syspage"

/* Kernel adresses and sizes */
#define KERNEL_OFFS     0x8000


/* phfs sources  */
#define PDN_FLASH0  0
#define PDN_FLASH1  1
#define PDN_COM1    2

#define PDN_NB      3

#define UART_LOADER_ID 3

/* Boot command size */
#define CMD_SIZE     64



#endif
