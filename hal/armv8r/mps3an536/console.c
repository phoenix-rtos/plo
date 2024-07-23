/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2024 Phoenix Systems
 * Authors: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>

#include <board_config.h>


#define TX_BUF_FULL (1 << 0)

#define CONCAT_(a, b) a##b
#define CONCAT(a, b)  CONCAT_(a, b)

#define UART_CONSOLE_BASE CONCAT(UART, CONCAT(UART_CONSOLE_PLO, _BASE))


static struct {
	volatile u32 *uart;
	ssize_t (*writeHook)(int, const void *, size_t);
} halconsole_common;


/* UART registers */
/* clang-format off */
enum { data = 0, state, ctrl, intstatus, bauddiv };
/* clang-format on */


void hal_consoleSetHooks(ssize_t (*writeHook)(int, const void *, size_t))
{
	halconsole_common.writeHook = writeHook;
}


void hal_consolePrint(const char *s)
{
	const char *ptr;

	for (ptr = s; *ptr != '\0'; ++ptr) {
		/* No hardware FIFO, wait until TX buffer is empty */
		while ((*(halconsole_common.uart + state) & TX_BUF_FULL) != 0) {
		}
		*(halconsole_common.uart + data) = *ptr;
	}

	if (halconsole_common.writeHook != NULL) {
		(void)halconsole_common.writeHook(0, s, ptr - s);
	}
}


void console_init(void)
{
	/* Set scaler */
	u32 scaler = SYSCLK_FREQ / UART_BAUDRATE;
	halconsole_common.uart = (void *)UART_CONSOLE_BASE;
	*(halconsole_common.uart + bauddiv) = scaler;
	hal_cpuDataSyncBarrier();

	/* Enable TX */
	*(halconsole_common.uart + ctrl) = 0x1;
}
