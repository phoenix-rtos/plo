/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Peripherals definitions for TDA4VM
 *
 * Copyright 2025 Phoenix Systems
 * Author: Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _PERIPHERALS_H_
#define _PERIPHERALS_H_

#include <board_config.h>
#include "tda4vm.h"


/* UARTs configuration */
#ifndef UART0_BAUDRATE
#define UART0_BAUDRATE 115200
#endif

#define UART_MAX_CNT 1


#define MCU_UART0_BASE_ADDR ((volatile u32 *)0x40a00000)
#define MCU_UART0_IRQ       30

#define MAIN_UART0_BASE_ADDR ((volatile u32 *)0x02800000)
#define MAIN_UART1_BASE_ADDR ((volatile u32 *)0x02810000)
#define MAIN_UART2_BASE_ADDR ((volatile u32 *)0x02820000)
#define MAIN_UART3_BASE_ADDR ((volatile u32 *)0x02830000)
#define MAIN_UART4_BASE_ADDR ((volatile u32 *)0x02840000)
#define MAIN_UART5_BASE_ADDR ((volatile u32 *)0x02850000)
#define MAIN_UART6_BASE_ADDR ((volatile u32 *)0x02860000)
#define MAIN_UART7_BASE_ADDR ((volatile u32 *)0x02870000)
#define MAIN_UART8_BASE_ADDR ((volatile u32 *)0x02880000)
#define MAIN_UART9_BASE_ADDR ((volatile u32 *)0x02890000)

/* MAIN domain UART interrupts have to be routed, the IRQ should be defined in board_config.h */
#ifndef MAIN_UART0_IRQ
#define MAIN_UART0_IRQ -1
#endif

#ifndef MAIN_UART1_IRQ
#define MAIN_UART1_IRQ -1
#endif

#ifndef MAIN_UART2_IRQ
#define MAIN_UART2_IRQ -1
#endif

#ifndef MAIN_UART3_IRQ
#define MAIN_UART3_IRQ -1
#endif

#ifndef MAIN_UART4_IRQ
#define MAIN_UART4_IRQ -1
#endif

#ifndef MAIN_UART5_IRQ
#define MAIN_UART5_IRQ -1
#endif

#ifndef MAIN_UART6_IRQ
#define MAIN_UART6_IRQ -1
#endif

#ifndef MAIN_UART7_IRQ
#define MAIN_UART7_IRQ -1
#endif

#ifndef MAIN_UART8_IRQ
#define MAIN_UART8_IRQ -1
#endif

#ifndef MAIN_UART9_IRQ
#define MAIN_UART9_IRQ -1
#endif

/* TIMERs configuration */

#define MCU_TIMER_BASE_ADDR(x) ((volatile u32 *)(0x40400000 + ((x) * 0x10000)))
#define MCU_TIMER0_INTR        38
#define MCU_TIMER1_INTR        39
#define MCU_TIMER2_INTR        40
#define MCU_TIMER3_INTR        41
#define MCU_TIMER4_INTR        108
#define MCU_TIMER5_INTR        109
#define MCU_TIMER6_INTR        110
#define MCU_TIMER7_INTR        111
#define MCU_TIMER8_INTR        112
#define MCU_TIMER9_INTR        113

#define TIMER_SRC_FREQ_HZ 250000000
#define TIMER_TICK_HZ     1000


#endif
