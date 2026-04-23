/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Peripherals definitions for armv8m33-stm32h5
 *
 * Copyright 2020, 2021, 2025, 2026 Phoenix Systems
 * Author: Hubert Buczynski, Aleksander Kaminski, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

/* Periperals configuration */

/* Interrupts */
#define SIZE_INTERRUPTS (134 + 16)


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
#define UART_MAX_CNT 12

#ifndef UART1
#define UART1 0
#endif

#ifndef UART2
#define UART2 0
#endif

#ifndef UART3
#define UART3 0
#endif

#ifndef UART4
#define UART4 0
#endif

#ifndef UART5
#define UART5 1
#endif

#ifndef UART6
#define UART6 0
#endif

#ifndef UART7
#define UART7 0
#endif

#ifndef UART8
#define UART8 0
#endif

#ifndef UART9
#define UART9 0
#endif

#ifndef UART10
#define UART10 0
#endif

#ifndef UART11
#define UART11 0
#endif

#ifndef UART12
#define UART12 0
#endif

#define UART_BAUDRATE 115200

#define UART1_BASE   ((void *)0x50013800)
#define UART2_BASE   ((void *)0x50004400)
#define UART3_BASE   ((void *)0x50004800)
#define UART4_BASE   ((void *)0x50004c00)
#define UART5_BASE   ((void *)0x50005000)
#define UART6_BASE   ((void *)0x50006400)
#define UART7_BASE   ((void *)0x50007800)
#define UART8_BASE   ((void *)0x50007c00)
#define UART9_BASE   ((void *)0x50008000)
#define UART10_BASE  ((void *)0x50006800)
#define UART11_BASE  ((void *)0x50006c00)
#define UART12_BASE  ((void *)0x50008400)

#define UART1_CLK   dev_usart1
#define UART2_CLK   dev_usart2
#define UART3_CLK   dev_usart3
#define UART4_CLK   dev_uart4
#define UART5_CLK   dev_uart5
#define UART6_CLK   dev_usart6
#define UART7_CLK   dev_uart7
#define UART8_CLK   dev_uart8
#define UART9_CLK   dev_uart9
#define UART10_CLK  dev_usart10
#define UART11_CLK  dev_usart11
#define UART12_CLK  dev_uart12

#define UART1_IRQ   (16 + 58)
#define UART2_IRQ   (16 + 59)
#define UART3_IRQ   (16 + 60)
#define UART4_IRQ   (16 + 61)
#define UART5_IRQ   (16 + 62)
#define UART6_IRQ   (16 + 85)
#define UART7_IRQ   (16 + 98)
#define UART8_IRQ   (16 + 99)
#define UART9_IRQ   (16 + 100)
#define UART10_IRQ  (16 + 86)
#define UART11_IRQ  (16 + 87)
#define UART12_IRQ  (16 + 101)

#endif
