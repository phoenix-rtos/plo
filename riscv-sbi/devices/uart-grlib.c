/*
 * Phoenix-RTOS
 *
 * Phoenix SBI
 *
 * GRLIB UART driver
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "hart.h"
#include "fdt.h"
#include "types.h"

#include "devices/console.h"


/* UART control bits */
#define RX_EN        (1 << 0)
#define TX_EN        (1 << 1)
#define TX_FIFO_FULL (1 << 9)

/* UART status bits */
#define DATA_READY (1 << 0)

enum {
	uart_data = 0, /* Data register           : 0x00 */
	uart_status,   /* Status register         : 0x04 */
	uart_ctrl,     /* Control register        : 0x08 */
	uart_scaler,   /* Scaler reload register  : 0x0C */
	uart_dbg       /* FIFO debug register     : 0x10 */
};


static struct {
	volatile u32 *base;
} uart_grlib_common;


static void uart_grlib_putc(char c)
{
	while ((*(uart_grlib_common.base + uart_status) & TX_FIFO_FULL) != 0) { }
	*(uart_grlib_common.base + uart_data) = c;
}


static int uart_grlib_getc(void)
{
	u32 st = *(uart_grlib_common.base + uart_status);
	if ((st & DATA_READY) == 0) {
		return -1;
	}

	return *(uart_grlib_common.base + uart_data) & 0xff;
}


static u32 uart_grlib_calcScaler(u32 freq, u32 baud)
{
	return (freq / (baud * 8 + 7));
}


static int uart_grlib_init(const char *compatible)
{
	uart_info_t info;
	if (fdt_getUartInfo(&info, compatible) < 0) {
		return -1;
	}
	uart_grlib_common.base = (vu32 *)info.reg.base;
	*(uart_grlib_common.base + uart_ctrl) = 0;
	RISCV_FENCE(w, o);
	*(uart_grlib_common.base + uart_scaler) = uart_grlib_calcScaler(info.freq, info.baud);
	RISCV_FENCE(w, o);
	*(uart_grlib_common.base + uart_ctrl) = TX_EN | RX_EN;

	return 0;
}


static const uart_driver_t uart_grlib __attribute__((section("uart_drivers"), used)) = {
	.compatible = "gaisler,apbuart",
	.init = uart_grlib_init,
	.putc = uart_grlib_putc,
	.getc = uart_grlib_getc
};
