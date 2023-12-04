/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Console
 *
 * Copyright 2022 Phoenix Systems
 * Authors: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include "nrf91.h"
#include <board_config.h>


/* max value for uarte_txd_maxcnt register */
#define TX_DMA_SIZE_MAX 8191


static struct {
	volatile u32 *base;
	u32 txDma;
	s32 txDmaSize;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


/* clang-format off */
enum { uarte_startrx = 0, uarte_stoprx, uarte_starttx, uarte_stoptx, uarte_flushrx = 11,
	uarte_events_cts = 64, uarte_events_rxdrdy = 66, uarte_events_endrx = 68, uarte_events_txdrdy = 71,
	uarte_events_endtx, uarte_events_error, uarte_events_rxto = 81, uarte_events_txstarted = 84,
	uarte_inten = 192, uarte_intenset, uarte_intenclr, uarte_errorsrc = 288, uarte_enable = 320,
	uarte_psel_rts = 322, uarte_psel_txd, uarte_psel_cts, uarte_psel_rxd, uarte_baudrate = 329,
	uarte_rxd_ptr = 333, uarte_rxd_maxcnt, uarte_rxd_amount, uarte_txd_ptr = 337, uarte_txd_maxcnt, uarte_txd_amount,
	uarte_config = 347 };
/* clang-format on */


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


void hal_consolePrint(const char *s)
{
	volatile char *tx_dma_buff = (volatile char *)halconsole_common.txDma;
	const char *ptr = s;
	int cnt;

	do {
		cnt = 0;
		while ((*ptr != '\0') && (cnt < halconsole_common.txDmaSize)) {
			tx_dma_buff[cnt] = *ptr;
			ptr++;
			cnt++;
		}

		if (cnt == 0) {
			break;
		}

		*(halconsole_common.base + uarte_txd_ptr) = halconsole_common.txDma;
		*(halconsole_common.base + uarte_txd_maxcnt) = cnt;
		*(halconsole_common.base + uarte_starttx) = 1u;

		while (*(halconsole_common.base + uarte_events_txstarted) != 1u) {
		}
		*(halconsole_common.base + uarte_events_txstarted) = 0u;

		while (*(halconsole_common.base + uarte_events_endtx) != 1u) {
		}
		*(halconsole_common.base + uarte_events_endtx) = 0u;

	} while (*ptr != '\0');

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


void console_init(void)
{
	static const struct {
		u8 uart;
		volatile u32 *base;
		u8 txPin;
		u8 rxPin;
		u8 rtsPin;
		u8 ctsPin;
		u32 txDma;
		u32 rxDma;
		u32 txDmaSize;
		u32 rxDmaSize;
	} uarts[] = {
		{ 0, (u32 *)UART0_BASE, UART0_TX, UART0_RX, UART0_RTS, UART0_CTS,
			UART0_TX_DMA, UART0_RX_DMA, TX_DMA_SIZE_MAX, UART_RX_DMA_SIZE },
		{ 1, (u32 *)UART1_BASE, UART1_TX, UART1_RX, UART1_RTS, UART1_CTS,
			UART1_TX_DMA, UART1_RX_DMA, TX_DMA_SIZE_MAX, UART_RX_DMA_SIZE },
		{ 2, (u32 *)UART2_BASE, UART2_TX, UART2_RX, UART2_RTS, UART2_CTS,
			UART2_TX_DMA, UART2_RX_DMA, TX_DMA_SIZE_MAX, UART_RX_DMA_SIZE },
		{ 3, (u32 *)UART3_BASE, UART3_TX, UART3_RX, UART3_RTS, UART3_CTS,
			UART3_TX_DMA, UART3_RX_DMA, TX_DMA_SIZE_MAX, UART_RX_DMA_SIZE }
	};

	halconsole_common.base = uarts[UART_CONSOLE_PLO].base;
	halconsole_common.txDma = uarts[UART_CONSOLE_PLO].txDma;
	halconsole_common.txDmaSize = uarts[UART_CONSOLE_PLO].txDmaSize;

	/* Config pins */
	_nrf91_gpioConfig(uarts[UART_CONSOLE_PLO].txPin, gpio_output, gpio_nopull);
	_nrf91_gpioConfig(uarts[UART_CONSOLE_PLO].rxPin, gpio_input, gpio_nopull);
	_nrf91_gpioConfig(uarts[UART_CONSOLE_PLO].rtsPin, gpio_output, gpio_nopull);
	_nrf91_gpioConfig(uarts[UART_CONSOLE_PLO].ctsPin, gpio_input, gpio_pulldown);

	_nrf91_gpioSet(uarts[UART_CONSOLE_PLO].txPin, gpio_high);
	_nrf91_gpioSet(uarts[UART_CONSOLE_PLO].rxPin, gpio_high);

	/* disable uarte instance */
	*(halconsole_common.base + uarte_enable) = 0u;
	hal_cpuDataMemoryBarrier();

	/* Select pins */
	*(halconsole_common.base + uarte_psel_txd) = uarts[UART_CONSOLE_PLO].txPin;
	*(halconsole_common.base + uarte_psel_rxd) = uarts[UART_CONSOLE_PLO].rxPin;
	*(halconsole_common.base + uarte_psel_rts) = uarts[UART_CONSOLE_PLO].rtsPin;
	*(halconsole_common.base + uarte_psel_cts) = uarts[UART_CONSOLE_PLO].ctsPin;

	/* currently supported baud rates: 9600, 115200 */
	*(halconsole_common.base + uarte_baudrate) = 0x01d60000u;

	/* Default settings - hardware flow control disabled, exclude parity bit, one stop bit */
	*(halconsole_common.base + uarte_config) = 0u;

	/* Set default max number of bytes in specific buffers */
	*(halconsole_common.base + uarte_txd_maxcnt) = uarts[UART_CONSOLE_PLO].txDmaSize;
	*(halconsole_common.base + uarte_rxd_maxcnt) = uarts[UART_CONSOLE_PLO].rxDmaSize;

	/* Set default memory regions for uart dma */
	*(halconsole_common.base + uarte_txd_ptr) = uarts[UART_CONSOLE_PLO].txDma;
	*(halconsole_common.base + uarte_rxd_ptr) = uarts[UART_CONSOLE_PLO].rxDma;

	/* disable all uart interrupts */
	*(halconsole_common.base + uarte_intenclr) = 0xffffffffu;
	hal_cpuDataMemoryBarrier();

	/* enable uarte instance */
	*(halconsole_common.base + uarte_enable) = 0x8u;
	hal_cpuDataMemoryBarrier();
}
