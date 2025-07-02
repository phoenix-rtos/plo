/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Peripherals definitions for armv8m55-stm32n6
 *
 * Copyright 2020, 2021, 2025 Phoenix Systems
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
#define SIZE_INTERRUPTS (195 + 16)


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
#define UART_MAX_CNT 10

#ifndef UART1
#define UART1 1
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
#define UART5 0
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

#define UART_BAUDRATE 115200

#define UART1_BASE   ((void *)0x52001000)
#define UART2_BASE   ((void *)0x50004400)
#define UART3_BASE   ((void *)0x50004800)
#define UART4_BASE   ((void *)0x50004c00)
#define UART5_BASE   ((void *)0x50005000)
#define UART6_BASE   ((void *)0x52001400)
#define UART7_BASE   ((void *)0x50007800)
#define UART8_BASE   ((void *)0x50007c00)
#define UART9_BASE   ((void *)0x52001800)
#define UART10_BASE  ((void *)0x52001c00)
#define LPUART1_BASE ((void *)0x56000c00)

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
#define LPUART1_CLK dev_lpuart1

#define UART1_IRQ   (16 + 159)
#define UART2_IRQ   (16 + 160)
#define UART3_IRQ   (16 + 161)
#define UART4_IRQ   (16 + 162)
#define UART5_IRQ   (16 + 163)
#define UART6_IRQ   (16 + 164)
#define UART7_IRQ   (16 + 165)
#define UART8_IRQ   (16 + 166)
#define UART9_IRQ   (16 + 167)
#define UART10_IRQ  (16 + 168)
#define LPUART1_IRQ (16 + 169)
#endif
