/*
 * Phoenix-RTOS
 *
 * Operating system loader
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
#define UART_BAUDRATE 115200
#define UART_REF_CLK  50000000 /* 50 MHz - description in _zynq_peripherals() */


#define UARTS_MAX_CNT 2


#define UART0_BASE_ADDR ((void *)0xe0000000)
#define UART1_BASE_ADDR ((void *)0xe0001000)

#define UART0_IRQ 59
#define UART1_IRQ 82

#define UART0_CLK amba_uart0_clk
#define UART1_CLK amba_uart1_clk


/* TIMERs configuration */
#define TTC0_BASE_ADDR ((void *)0xf8001000)
#define TTC1_BASE_ADDR ((void *)0xf8002000)

#define TTC0_1_IRQ 42
#define TTC0_2_IRQ 43
#define TTC0_3_IRQ 44

#define TTC1_1_IRQ 69
#define TTC1_2_IRQ 70
#define TTC1_3_IRQ 71


/* USB OTG */
#define USB0_BASE_ADDR ((void *)0xE0002000)
#define USB1_BASE_ADDR ((void *)0xE0003000)

#define USB0_IRQ 53
#define USB1_IRQ 76

#define PHFS_ACM_PORTS_NB 1 /* Number of ports define by CDC driver; min = 1, max = 2 */


/* GPIO */
#define GPIO_BASE_ADDR ((void *)0xe000a000)

#endif
