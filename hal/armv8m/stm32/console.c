/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Console
 *
 * Copyright 2021, 2025 Phoenix Systems
 * Copyright 2026 Apator Metrix
 * Authors: Aleksander Kaminski, Jacek Maksymowicz, Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/helpers.h>

#include <board_config.h>

#include "stm32.h"

#if !ISEMPTY(UART_CONSOLE_PLO)

#if (UART_CONSOLE_PLO <= 0)
#error "UART_CONSOLE_PLO set incorrectly"
#endif


#define CONCAT_(a, b) a##b
#define CONCAT(a, b)  CONCAT_(a, b)

#define UART_IO_PORT_DEV CONCAT(dev_, UART_IO_PORT)

#define UART_ISR_TC  (1 << 6)
#define UART_ISR_TXE (1 << 7)


/* Values for selecting the peripheral clock for an UART */
enum {
	uart_clk_sel_pclk = 0, /* pclk1 or pclk2 depending on peripheral */
#if defined(__CPU_STM32N6)
	uart_clk_sel_per_ck,
	uart_clk_sel_ic9_ck,
	uart_clk_sel_ic14_ck,
	uart_clk_sel_lse_ck,
	uart_clk_sel_msi_ck,
	uart_clk_sel_hsi_div_ck,
#elif defined(__CPU_STM32U5)
	uart_clk_sel_sysclk,
	uart_clk_sel_hsi_ck,
	uart_clk_sel_lse_ck,
#endif
};


/* clang-format off */
enum { cr1 = 0, cr2, cr3, brr, gtpr, rtor, rqr, isr, icr, rdr, tdr, presc };
/* clang-format on */


static struct {
	volatile u32 *base;
	unsigned refclkfreq;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


void hal_consolePrint(const char *s)
{
	const char *ptr;

	for (ptr = s; *ptr != '\0'; ++ptr) {
		while (((*(halconsole_common.base + isr)) & UART_ISR_TXE) == 0) {
			/* Wait until transmit data register is empty */
		}

		*(halconsole_common.base + tdr) = *ptr;
	}

	while (((*(halconsole_common.base + isr)) & UART_ISR_TC) == 0) {
		/* Wait until transmission is complete */
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


void console_init(void)
{
	static const struct {
		void *base;
		u16 dev_clk;
		u8 ipclk_sel;
	} uarts[] = {
		{ UART1_BASE, UART1_CLK, ipclk_usart1sel },
		{ UART2_BASE, UART2_CLK, ipclk_usart2sel },
		{ UART3_BASE, UART3_CLK, ipclk_usart3sel },
		{ UART4_BASE, UART4_CLK, ipclk_uart4sel },
		{ UART5_BASE, UART5_CLK, ipclk_uart5sel },
#if defined(__CPU_STM32N6)
		{ UART6_BASE, UART6_CLK, ipclk_usart6sel },
		{ UART7_BASE, UART7_CLK, ipclk_uart7sel },
		{ UART8_BASE, UART8_CLK, ipclk_uart8sel },
		{ UART9_BASE, UART9_CLK, ipclk_uart9sel },
		{ UART10_BASE, UART10_CLK, ipclk_usart10sel },
#endif
	};

	const int uart = UART_CONSOLE_PLO - 1, port = UART_IO_PORT_DEV, txpin = UART_PIN_TX, rxpin = UART_PIN_RX, af = UART_IO_AF;

	_stm32_rccSetDevClock(port, 1);

	halconsole_common.base = uarts[uart].base;

	/* Init tx pin - output, push-pull, low speed, no pull-up */
	_stm32_gpioConfig(port, txpin, gpio_mode_af, af, gpio_otype_pp, gpio_ospeed_low, gpio_pupd_nopull);

	/* Init rxd pin - input, push-pull, low speed, no pull-up */
	_stm32_gpioConfig(port, rxpin, gpio_mode_af, af, gpio_otype_pp, gpio_ospeed_low, gpio_pupd_nopull);

#if defined(__CPU_STM32N6)
	_stm32_rccSetIPClk(uarts[uart].ipclk_sel, uart_clk_sel_hsi_div_ck);
	halconsole_common.refclkfreq = 64 * 1000 * 1000;
#elif defined(__CPU_STM32U5)
	_stm32_rccSetIPClk(uarts[uart].ipclk_sel, uart_clk_sel_sysclk);
	halconsole_common.refclkfreq = _stm32_rccGetCPUClock();
#endif

	/* Enable uart clock */
	_stm32_rccSetDevClock(uarts[uart].dev_clk, 1);

	/* Set up UART to 115200,8,n,1 16-bit oversampling */
	*(halconsole_common.base + cr1) &= ~1UL; /* disable USART */
	hal_cpuDataMemoryBarrier();
	*(halconsole_common.base + cr1) = 0xa;
	*(halconsole_common.base + cr2) = 0;
	*(halconsole_common.base + cr3) = 0;
	*(halconsole_common.base + brr) = halconsole_common.refclkfreq / 115200; /* 115200 baud rate */
	hal_cpuDataMemoryBarrier();
	*(halconsole_common.base + cr1) |= 1;
	hal_cpuDataMemoryBarrier();
}

#else

void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	(void)writeHook;
}


void hal_consolePrint(const char *s)
{
	(void)s;
}


void console_init(void)
{
}

#endif /* #if !ISEMPTY(UART_CONSOLE_PLO) */
