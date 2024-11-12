/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Peripherals definitions for ZynqMP
 *
 * Copyright 2024 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include "zynqmp.h"


/* UARTs configuration */
#define UART_BAUDRATE 115200
#define UART_REF_CLK  49995000 /* close to 50 MHz - description in uart_initCtrlClock() */


#define UARTS_MAX_CNT 2


#define UART0_BASE_ADDR ((void *)0x00ff000000)
#define UART1_BASE_ADDR ((void *)0x00ff010000)

#define UART0_IRQ 53
#define UART1_IRQ 54

#define UART0_PLL   ctl_clock_dev_lpd_uart0
#define UART1_PLL   ctl_clock_dev_lpd_uart1
#define UART0_RESET ctl_reset_lpd_uart0
#define UART1_RESET ctl_reset_lpd_uart1


/* TIMERs configuration */
#define TTC0_BASE_ADDR ((void *)0x00ff110000)
#define TTC1_BASE_ADDR ((void *)0x00ff120000)

#define TTC0_1_IRQ 68
#define TTC0_2_IRQ 69
#define TTC0_3_IRQ 70

#define TTC1_1_IRQ 71
#define TTC1_2_IRQ 72
#define TTC1_3_IRQ 73


/* USB OTG */
#define USB3_0_BASE_ADDR      ((void *)0x00ff9d0000)
#define USB3_1_BASE_ADDR      ((void *)0x00ff9e0000)
#define USB3_0_XHCI_BASE_ADDR ((void *)0x00fe200000)
#define USB3_1_XHCI_BASE_ADDR ((void *)0x00fe300000)


#define USB0_IRQ_BASE 97
#define USB1_IRQ_BASE 102

#define PHFS_ACM_PORTS_NB 1 /* Number of ports define by CDC driver; min = 1, max = 2 */


/* GPIO */
#define GPIO_BASE_ADDR ((void *)0x00ff0a0000)


/* QSPI*/
#define QSPI_BASE_ADDR ((void *)0x00ff0f0000)

#endif
