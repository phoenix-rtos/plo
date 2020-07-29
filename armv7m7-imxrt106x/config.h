/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Configuration for armv7m7-imxrt106x
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VERSION      "1.0.1"


/* User interface */
#define WELCOME      "-\\- Phoenix-RTOS loader for i. MX RT106x, version: " VERSION

#define DEFAULT_CMD "load flash0; go!"


/* Kernel adresses and sizes */
#define KERNEL_BASE  0x14000


/* phfs sources  */
#define PDN_FLASH0  0
#define PDN_FLASH1  1
#define PDN_COM1    2
#define PDN_COM2    3
#define PDN_USB     4

#define PDN_NB      2

/* Boot command size */
#define CMD_SIZE     64


/* Periperals configuration */

/* UART */
#ifndef UART1
#define UART1 0
#endif

#ifndef UART2
#define UART2 0
#endif

#ifndef UART3
#define UART3 1
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

#ifndef UART_CONSOLE
#define UART_CONSOLE 3
#endif

#define UART_CLK 80000000

#define UART1_BASE ((void *)0x40184000)
#define UART2_BASE ((void *)0x40188000)
#define UART3_BASE ((void *)0x4018c000)
#define UART4_BASE ((void *)0x40190000)
#define UART5_BASE ((void *)0x40194000)
#define UART6_BASE ((void *)0x40198000)
#define UART7_BASE ((void *)0x4019c000)
#define UART8_BASE ((void *)0x401a0000)

#define UART1_CLK pctl_clk_lpuart1
#define UART2_CLK pctl_clk_lpuart2
#define UART3_CLK pctl_clk_lpuart3
#define UART4_CLK pctl_clk_lpuart4
#define UART5_CLK pctl_clk_lpuart5
#define UART6_CLK pctl_clk_lpuart6
#define UART7_CLK pctl_clk_lpuart7
#define UART8_CLK pctl_clk_lpuart8

#define UART1_IRQ 20 + 16
#define UART2_IRQ 21 + 16
#define UART3_IRQ 22 + 16
#define UART4_IRQ 23 + 16
#define UART5_IRQ 24 + 16
#define UART6_IRQ 25 + 16
#define UART7_IRQ 26 + 16
#define UART8_IRQ 27 + 16


#define UART1_TX_PIN ad_b0_12
#define UART1_RX_PIN ad_b0_13
#define UART1_RTS_PIN ad_b0_15
#define UART1_CTS_PIN ad_b0_14

#ifndef UART2_TX_PIN
#define UART2_TX_PIN ad_b1_02
//#define UART2_TX_PIN sd_b1_11
#endif
#ifndef UART2_RX_PIN
#define UART2_RX_PIN ad_b1_03
//#define UART2_RX_PIN sd_b1_10
#endif
#define UART2_RTS_PIN ad_b1_01
#define UART2_CTS_PIN ad_b1_00

#ifndef UART3_TX_PIN
#define UART3_TX_PIN emc_13
//#define UART3_TX_PIN ad_b1_06
//#define UART3_TX_PIN b0_08
#endif
#ifndef UART3_RX_PIN
#define UART3_RX_PIN emc_14
//#define UART3_RX_PIN ad_b1_07
//#define UART3_RX_PIN b0_09
#endif
#ifndef UART3_RTS_PIN
#define UART3_RTS_PIN emc_16
//#define UART3_RTS_PIN ad_b1_05
#endif
#ifndef UART3_CTS_PIN
#define UART3_CTS_PIN emc_15
//#define UART3_CTS_PIN ad_b1_04
#endif

#ifndef UART4_TX_PIN
#define UART4_TX_PIN emc_19
//#define UART4_TX_PIN b1_00
//#define UART4_TX_PIN sd_b1_00
#endif
#ifndef UART4_RX_PIN
#define UART4_RX_PIN emc_20
//#define UART4_RX_PIN b1_01
//#define UART4_RX_PIN sd_b1_01
#endif
#define UART4_RTS_PIN emc_18
#define UART4_CTS_PIN emc_17

#ifndef UART5_TX_PIN
#define UART5_TX_PIN emc_23
//#define UART5_TX_PIN b1_12
#endif
#ifndef UART5_RX_PIN
#define UART5_RX_PIN emc_24
//#define UART5_RX_PIN b1_13
#endif
#define UART5_RTS_PIN emc_27
#define UART5_CTS_PIN emc_28

#ifndef UART6_TX_PIN
#define UART6_TX_PIN emc_25
//#define UART6_TX_PIN ad_b0_12
#endif
#ifndef UART6_RX_PIN
#define UART6_RX_PIN emc_26
//#define UART6_RX_PIN ad_b0_03
#endif
#define UART6_RTS_PIN emc_29
#define UART6_CTS_PIN emc_30

#ifndef UART7_TX_PIN
#define UART7_TX_PIN emc_31
//#define UART7_TX_PIN sd_b1_08
#endif
#ifndef UART7_RX_PIN
#define UART7_RX_PIN emc_32
//#define UART7_RX_PIN sd_b1_09
#endif
#define UART7_RTS_PIN sd_b1_07
#define UART7_CTS_PIN sd_b1_06

#ifndef UART8_TX_PIN
#define UART8_TX_PIN emc_38
//#define UART8_TX_PIN ad_b1_10
//#define UART8_TX_PIN sd_b0_04
#endif
#ifndef UART8_RX_PIN
#define UART8_RX_PIN emc_39
//#define UART8_RX_PIN ad_b1_11
//#define UART8_RX_PIN sd_b0_05
#endif
#define UART8_RTS_PIN sd_b0_03
#define UART8_CTS_PIN sd_b0_02



/* GPT - general purpose timer */

#define GPT1_BASE           ((void *)0x401ec000)
#define GPT1_CLK            pctl_clk_gpt1_bus
#define GPT1_IRQ            100 + 16

#define GPT2_BASE           ((void *)0x401f0000)
#define GPT2_CLK            pctl_clk_gpt2_bus
#define GPT2_IRQ            101 + 16

#endif
