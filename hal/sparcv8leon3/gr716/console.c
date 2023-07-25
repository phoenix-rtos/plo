/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Console
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <devices/gpio-gr716/gpio.h>


/* UART control bits */
#define TX_EN        (1 << 1)
#define TX_FIFO_FULL (1 << 9)

/* Console config */
#define UART_CONSOLE_RX   UART2_RX
#define UART_CONSOLE_TX   UART2_TX
#define UART_CONSOLE_BASE UART2_BASE
#define UART_CONSOLE_CGU  cgudev_apbuart2


enum {
	uart_data,   /* Data register           : 0x00 */
	uart_status, /* Status register         : 0x04 */
	uart_ctrl,   /* Control register        : 0x08 */
	uart_scaler, /* Scaler reload register  : 0x0C */
	uart_dbg     /* FIFO debug register     : 0x10 */
};


static struct {
	volatile u32 *uart;
} halconsole_common;


void hal_consolePrint(const char *s)
{
	for (; *s; s++) {
		/* Wait until TX fifo is not full */
		while ((*(halconsole_common.uart + uart_status) & TX_FIFO_FULL) != 0) {
		}
		*(halconsole_common.uart + uart_data) = *s;
	}
}


static u32 console_calcScaler(u32 baud)
{
	u32 scaler = (SYSCLK_FREQ / (baud * 8 + 7));
	return scaler;
}


void console_init(void)
{
	iomux_cfg_t cfg;

	cfg.opt = 0x1;
	cfg.pullup = 0;
	cfg.pulldn = 0;
	cfg.pin = UART_CONSOLE_TX;
	_gr716_iomuxCfg(&cfg);

	cfg.pin = UART_CONSOLE_RX;
	_gr716_iomuxCfg(&cfg);

	_gr716_cguClkEnable(cgu_primary, UART_CONSOLE_CGU);
	halconsole_common.uart = UART_CONSOLE_BASE;
	*(halconsole_common.uart + uart_ctrl) = TX_EN;
	*(halconsole_common.uart + uart_scaler) = console_calcScaler(UART_BAUDRATE);
	hal_cpuDataStoreBarrier();
}
