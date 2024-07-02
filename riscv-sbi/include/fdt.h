/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * FDT parsing
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SBI_FDT_H_
#define _SBI_FDT_H_


#include "devices/clint.h"
#include "devices/console.h"


int fdt_parseCpus(void);


int fdt_getUartInfo(uart_info_t *uart, const char *compatible);


int fdt_getClintInfo(clint_info_t *clint);


void fdt_init(const void *fdt);


#endif
