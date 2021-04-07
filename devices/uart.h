/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * UART driver API
 *
 * Copyright 2012, 2020 Phoenix Systems
 * Copyright 2001, 2005 Pawel Pisarczyk
 * Author: Pawel Pisarczyk, Lukasz Kosinski, Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "../types.h"

extern void uart_init(void);

extern u32 uart_getBaudrate(void);

extern int uart_read(unsigned int pn, u8 *buff, u16 len, u16 timeout);

extern int uart_write(unsigned int pn, const u8 *buff, u16 len);

extern int uart_safewrite(unsigned int pn, const u8 *buff, u16 len);

extern int uart_rxEmpty(unsigned int pn);

extern void uart_done(void);


#endif
