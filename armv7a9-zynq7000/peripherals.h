/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Peripherals definitions for zynq-7000
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_


/* Zynq-7000 System Adress Map */
#define ADDR_OCRAM_LOW   0x00000000
#define SIZE_OCRAM_LOW   192 * 1024
#define ADDR_OCRAM_HIGH  0xffff0000
#define SIZE_OCRAM_HIGH  64 * 1024

#define ADDR_DDR         0x00100000
#define SIZE_DDR         512 * 1024 * 1024


/* UARTs configuration */
#define UART0_BASE_ADDR  ((void *)0xe0000000)
#define UART1_BASE_ADDR  ((void *)0xe0001000)

#define UART0_IRQ        59
#define UART1_IRQ        82

#define UART_BAUDRATE    115200
#define UART_REF_CLK     50000000    /* 50 MHz - description in _zynq_peripherals() */

#define UART_CONSOLE     1
#define UARTS_MAX_CNT    2


/* TIMERs configuration */
#define TTC0_BASE_ADDR   ((void *)0xf8001000)
#define TTC1_BASE_ADDR   ((void *)0xf8002000)


#endif