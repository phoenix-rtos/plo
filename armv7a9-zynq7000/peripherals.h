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

#include "zynq.h"


/* UARTs configuration */
#define UART_BAUDRATE    115200
#define UART_REF_CLK     50000000    /* 50 MHz - description in _zynq_peripherals() */

#define UART_CONSOLE     1
#define UARTS_MAX_CNT    2


#define UART0_BASE_ADDR  ((void *)0xe0000000)
#define UART1_BASE_ADDR  ((void *)0xe0001000)

#define UART0_IRQ        59
#define UART1_IRQ        82

#define UART0_CLK        amba_uart0_clk
#define UART1_CLK        amba_uart1_clk

#define UART0_RX         mio_pin_10
#define UART0_TX         mio_pin_11

#define UART1_RX         mio_pin_49
#define UART1_TX         mio_pin_48



/* TIMERs configuration */
#define TTC0_BASE_ADDR   ((void *)0xf8001000)
#define TTC1_BASE_ADDR   ((void *)0xf8002000)

#define TTC0_1_IRQ       42
#define TTC0_2_IRQ       43
#define TTC0_3_IRQ       44

#define TTC1_1_IRQ       69
#define TTC1_2_IRQ       70
#define TTC1_3_IRQ       71


/* USB OTG */
#define USB0_BASE_ADDR   ((void *)0xE0002000)
#define USB1_BASE_ADDR   ((void *)0xE0003000)

#define USB0_IRQ         53
#define USB1_IRQ         76


/* GPIO */
#define GPIO_BASE_ADDR  ((void *)0xe000a000)

#endif
