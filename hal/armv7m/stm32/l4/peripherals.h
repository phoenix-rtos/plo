/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Peripherals definitions for armv7m7-stm32l4x6
 *
 * Copyright 2020, 2021 Phoenix Systems
 * Author: Hubert Buczynski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

/* Periperals configuration */

/* Interrupts */
#define SIZE_INTERRUPTS (217 + 16)

/* UART */
#define UART_MAX_CNT 5

#ifndef UART1
#define UART1 0
#endif

#ifndef UART2
#define UART2 1
#endif

#ifndef UART3
#define UART3 0
#endif

#ifndef UART4
#define UART4 0
#endif

#ifndef UART5
#define UART5 0
#endif

#ifndef UART_CONSOLE
#define UART_CONSOLE 2
#endif

#define UART_BAUDRATE 115200

#define UART1_BASE ((void *)0x40013800)
#define UART2_BASE ((void *)0x40004400)
#define UART3_BASE ((void *)0x40004800)
#define UART4_BASE ((void *)0x40004c00)
#define UART5_BASE ((void *)0x40005000)

#define UART1_CLK pctl_usart1
#define UART2_CLK pctl_usart2
#define UART3_CLK pctl_usart3
#define UART4_CLK pctl_uart4
#define UART5_CLK pctl_uart5

#define UART1_IRQ usart1_irq
#define UART2_IRQ usart2_irq
#define UART3_IRQ usart3_irq
#define UART4_IRQ uart4_irq
#define UART5_IRQ uart5_irq

#endif
