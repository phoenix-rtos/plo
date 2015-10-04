/* 
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * UART 16450 driver
 *
 * Copyright 2001, 2005 Pawel Pisarczyk
 *
 * This file is part of Phoenix-RTOS.
 *
 * Phoenix-RTOS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Phoenix-RTOS kernel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phoenix-RTOS kernel; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _SERIAL_H_
#define _SERIAL_H_


/* UART registers */
#define REG_RBR     0
#define REG_THR     0
#define REG_IMR     1
#define REG_IIR     2
#define REG_LCR     3
#define REG_MCR     4
#define REG_LSR     5
#define REG_MSR     6
#define REG_ADR     7
#define REG_LSB     0
#define REG_MSB     1


/* Register bits */
#define IMR_THRE      0x02
#define IMR_DR        0x01

#define IIR_IRQPEND   0x01
#define IIR_THRE      0x02
#define IIR_DR        0x04

#define LCR_DLAB      0x80
#define LCR_D8N1      0x03
#define LCR_D8N2      0x07

#define MCR_OUT2      0x08

#define LSR_DR        0x01
#define LSR_THRE      0x20


#define COM1_BASE   0x3f8
#define COM1_IRQ    4
#define COM2_BASE   0x2f8
#define COM2_IRQ    3


#define BPS_28800    4
#define BPS_38400    3
#define BPS_57600    2
#define BPS_115200   1


/* Driver data sizes */
#define NPORTS    2
#define RBUFFSZ   2048
#define SBUFFSZ   2048


extern int serial_read(unsigned int pn, u8 *buff, u16 len, u16 timeout);

extern int serial_write(unsigned int pn, u8 *buff, u16 len);

extern void serial_init(u16 speed);

extern void serial_done(void);


#endif
