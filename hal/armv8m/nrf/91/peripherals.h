/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Peripherals definitions for armv8m33-nrf9160
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include <board_config.h>

/* Periperals configuration */

/* Based on INTLINESNUM value (ICTR cpu register) */
#define SIZE_INTERRUPTS 256

/* TIMER */
/* nrf9160 provides instances from 0 to 2 */
#ifndef TIMER_INSTANCE
#define TIMER_INSTANCE 0
#endif

/* UART */
#define UART_MAX_CNT 4

/* supported variants: uart0 + uart1 / uart2 + uart3 */
#ifndef UART0
#define UART0 1
#endif

#ifndef UART1
#define UART1 1
#endif

#ifndef UART2
#define UART2 0
#endif

#ifndef UART3
#define UART3 0
#endif

#define UART0_BASE 0x50008000u
#define UART1_BASE 0x50009000u
#define UART2_BASE 0x5000a000u
#define UART3_BASE 0x5000b000u

#define UART0_IRQ (uarte0 + 16)
#define UART1_IRQ (uarte1 + 16)
#define UART2_IRQ (uarte2 + 16)
#define UART3_IRQ (uarte3 + 16)

/* default uart0 dma regions ram7: section 2 and 3 */
#ifndef UART0_TX_DMA
#define UART0_TX_DMA 0x2003c000u
#endif
#ifndef UART0_RX_DMA
#define UART0_RX_DMA 0x2003e000u
#endif

/* default uart1 dma regions ram7: section 0 and 1 */
#ifndef UART1_TX_DMA
#define UART1_TX_DMA 0x20038000u
#endif
#ifndef UART1_RX_DMA
#define UART1_RX_DMA 0x2003a000u
#endif

/* default uart2 dma regions ram7: section 2 and 3 */
#ifndef UART2_TX_DMA
#define UART2_TX_DMA 0x2003c000u
#endif
#ifndef UART2_RX_DMA
#define UART2_RX_DMA 0x2003e000u
#endif

/* default uart3 dma regions ram7: section 0 and 1 */
#ifndef UART3_TX_DMA
#define UART3_TX_DMA 0x20038000u
#endif
#ifndef UART3_RX_DMA
#define UART3_RX_DMA 0x2003a000u
#endif

#define FLASH_PROGRAM_1_ADDR    0x00000000u
#define FLASH_PROGRAM_BANK_SIZE (1024 * 1024)

#endif
