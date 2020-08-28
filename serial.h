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


extern void serial_init(u32 baud, u32 *st);

extern int serial_read(unsigned int pn, u8 *buff, u16 len, u16 timeout);

extern int serial_write(unsigned int pn, const u8 *buff, u16 len);

extern int serial_safewrite(unsigned int pn, const u8 *buff, u16 len);

extern int serial_rxEmpty(unsigned int pn);

extern void serial_done(void);


#endif
