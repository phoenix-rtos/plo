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


static int console_setPin(u8 pin)
{
	int err = 0;
	io_cfg_t uartCfg;
	uartCfg.opt = 0x1;
	uartCfg.pin = pin;
	uartCfg.pullup = 0;
	uartCfg.pulldn = 0;

	switch (pin) {
		case UART_CONSOLE_TX:
			uartCfg.dir = GPIO_DIR_OUT;
			break;
		case UART_CONSOLE_RX:
			uartCfg.dir = GPIO_DIR_IN;
			break;
		default:
			err = -1;
			break;
	}

	return err == 0 ? _gr716_ioCfg(&uartCfg) : err;
}


static u32 console_calcScaler(u32 baud)
{
	u32 scaler = (SYSCLK_FREQ / (baud * 8 + 7));
	return scaler;
}


void console_init(void)
{
	console_setPin(UART_CONSOLE_TX);
	console_setPin(UART_CONSOLE_RX);
	_gr716_cguClkEnable(cgu_primary, UART_CONSOLE_CGU);
	halconsole_common.uart = UART_CONSOLE_BASE;
	*(halconsole_common.uart + uart_ctrl) = TX_EN;
	*(halconsole_common.uart + uart_scaler) = console_calcScaler(UART_BAUDRATE);
	hal_cpuDataStoreBarrier();
}
