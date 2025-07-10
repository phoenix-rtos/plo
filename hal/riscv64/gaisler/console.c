/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

/* UART control bits */
#define TX_EN        (1 << 1)
#define TX_FIFO_FULL (1 << 9)

enum {
	uart_data = 0, /* Data register           : 0x00 */
	uart_status,   /* Status register         : 0x04 */
	uart_ctrl,     /* Control register        : 0x08 */
	uart_scaler,   /* Scaler reload register  : 0x0C */
	uart_dbg       /* FIFO debug register     : 0x10 */
};


static struct {
	volatile u32 *uart;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


void hal_consolePrint(const char *s)
{
	const char *ptr = s;
	for (ptr = s; *ptr != '\0'; ++ptr) {
		/* Wait until TX fifo is not full */
		while ((*(halconsole_common.uart + uart_status) & TX_FIFO_FULL) != 0) {
		}
		*(halconsole_common.uart + uart_data) = *ptr;
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


static u32 console_calcScaler(u32 baud)
{
	return (SYSCLK_FREQ / (baud * 8 + 7));
}


void console_init(void)
{
	halconsole_common.uart = UART0_BASE;
	*(halconsole_common.uart + uart_ctrl) = 0;
	hal_cpuDataStoreBarrier();
	*(halconsole_common.uart + uart_scaler) = console_calcScaler(UART_BAUDRATE);
	hal_cpuDataStoreBarrier();
	*(halconsole_common.uart + uart_ctrl) = TX_EN;
}
