/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Peripherals definitions
 *
 * Copyright 2021 Phoenix Systems
 * Author: Lukasz Kosinski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_


/* DISK-BIOS configuration (floppy, hd0, hd1, hd2, hd3) */
#define DISKBIOS_FLOPPY_CNT 1
#define DISKBIOS_HD_CNT     4
#define DISKBIOS_MAX_CNT    (DISKBIOS_FLOPPY_CNT + DISKBIOS_HD_CNT)


/* TTY-BIOS configuration (VGA and keyboard based terminal) */
#define TTYBIOS_MAX_CNT 1


/* UARTs configuration (COM1, COM2) */
#define UART_MAX_CNT  2
#define UART_BAUDRATE 115200

/* Standard PC COMs */
#define UART_COM1 0x3f8
#define UART_COM2 0x2f8

#define UART_COM1_IRQ 4
#define UART_COM2_IRQ 3

/* Don't use COM3 and COM4 (no shared IRQs support) */
/* #define UART_COM3 0x3e8 */
/* #define UART_COM4 0x2e8 */

/* #define UART_COM3_IRQ 4 */
/* #define UART_COM4_IRQ 3 */

/* Intel Galileo UARTs */
#define UART_GALILEO1     0x9000f000
#define UART_GALILEO1_IRQ 17

/* Don't use GALILEO2 (no shared IRQs support) */
/* #define UART_GALILEO2     0x9000b000 */
/* #define UART_GALILEO2_IRQ 17 */


#endif
