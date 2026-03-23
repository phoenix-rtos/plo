/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Peripherals definitions for armv8m33-stm32u3
 *
 * Copyright 2020, 2021, 2025 Phoenix Systems
 * Copyright 2026 Apator Metrix
 * Author: Hubert Buczynski, Aleksander Kaminski, Jacek Maksymowicz, Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

/* Periperals configuration */

/* Interrupts */
#define SIZE_INTERRUPTS (125 + 16)


/* DEBUG - RTT PIPE */

#ifndef RTT_ENABLED_PLO
#define RTT_ENABLED_PLO 0
#endif

#ifndef RTT_BUFSZ_CONSOLE_TX
#define RTT_BUFSZ_CONSOLE_TX 1024
#endif

#ifndef RTT_BUFSZ_CONSOLE_RX
#define RTT_BUFSZ_CONSOLE_RX 1024
#endif

#ifndef RTT_BUFSZ_PHOENIXD_TX
#define RTT_BUFSZ_PHOENIXD_TX 1024
#endif

#ifndef RTT_BUFSZ_PHOENIXD_RX
#define RTT_BUFSZ_PHOENIXD_RX 1024
#endif


/* UART */
#define UART_MAX_CNT 5

#ifndef UART1
#ifdef TTY1
#define UART1 TTY1
#else
#define UART1 1
#endif
#endif

#ifndef UART2 /* STM32U396xx / STM32U3A6xx / STM32U3B5xx / STM32U3C5xx */
#ifdef TTY2
#define UART2 TTY2
#else
#define UART2 0
#endif
#endif

#ifndef UART3
#ifdef TTY3
#define UART3 TTY3
#else
#define UART3 0
#endif
#endif

#ifndef UART4
#ifdef TTY4
#define UART4 TTY4
#else
#define UART4 0
#endif
#endif

#ifndef UART5
#ifdef TTY5
#define UART5 TTY5
#else
#define UART5 0
#endif
#endif

#define UART_BAUDRATE 115200

#define UART1_BASE ((void *)0x40013800)
#define UART2_BASE ((void *)0x40004400)
#define UART3_BASE ((void *)0x40004800)
#define UART4_BASE ((void *)0x40004c00)
#define UART5_BASE ((void *)0x40005000)

#define UART1_CLK dev_usart1
#define UART2_CLK dev_usart2
#define UART3_CLK dev_usart3
#define UART4_CLK dev_uart4
#define UART5_CLK dev_uart5

#define UART1_IRQ (16 + 61)
#define UART2_IRQ (16 + 62)
#define UART3_IRQ (16 + 63)
#define UART4_IRQ (16 + 64)
#define UART5_IRQ (16 + 65)
#endif
