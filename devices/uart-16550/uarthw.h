/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * UART 16550 (Hardware Abstraction Layer)
 *
 * Copyright 2020, 2021 Phoenix Systems
 * Author: Pawel Pisarczyk, Lukasz Kosinski
 * 
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _DEV_UARTHW_H_
#define _DEV_UARTHW_H_


/* UART hardware context size */
#define SIZE_UARTHW_CTX 16


/* Reads from UART register */
extern unsigned char uarthw_read(void *hwctx, unsigned int reg);


/* Writes to UART register */
extern void uarthw_write(void *hwctx, unsigned int reg, unsigned char val);


/* Returns UART interrupt number */
extern unsigned int uarthw_irq(void *hwctx);


/* Initializes UART hardware context (optionally sets preferred baudrate index) */
extern int uarthw_init(unsigned int n, void *hwctx, unsigned int *baud);


#endif
