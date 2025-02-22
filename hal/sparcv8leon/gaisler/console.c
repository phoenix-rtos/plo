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

/* UART control bits */
#define TX_EN        (1 << 1)
#define TX_FIFO_FULL (1 << 9)

#define CONCAT_(a, b) a##b
#define CONCAT(a, b)  CONCAT_(a, b)

/* Console config */
#define UART_CONSOLE_RX   CONCAT(UART, CONCAT(UART_CONSOLE_PLO, _RX))
#define UART_CONSOLE_TX   CONCAT(UART, CONCAT(UART_CONSOLE_PLO, _TX))
#define UART_CONSOLE_BASE CONCAT(UART, CONCAT(UART_CONSOLE_PLO, _BASE))
#define UART_CONSOLE_CGU  CONCAT(cgudev_apbuart, UART_CONSOLE_PLO)


enum {
	uart_data = 0, /* Data register           : 0x00 */
	uart_status,   /* Status register         : 0x04 */
	uart_ctrl,     /* Control register        : 0x08 */
	uart_scaler,   /* Scaler reload register  : 0x0C */
	uart_dbg       /* FIFO debug register     : 0x10 */
};


static struct {
	volatile u32 *uart;
	void (*writeHook)(int, const void *, size_t);
} halconsole_common;


/* CPU-specific functions */

#if defined(__CPU_GR716)

static void console_cguClkEnable(void)
{
	_gr716_cguClkEnable(cgu_primary, UART_CONSOLE_CGU);
}


static int console_cguClkStatus(void)
{
	return _gr716_cguClkStatus(cgu_primary, UART_CONSOLE_CGU);
}


static void console_iomuxCfg(void)
{
	iomux_cfg_t cfg;

	cfg.opt = 0x1;
	cfg.pullup = 0;
	cfg.pulldn = 0;
	cfg.pin = UART_CONSOLE_TX;
	gaisler_iomuxCfg(&cfg);

	cfg.pin = UART_CONSOLE_RX;
	gaisler_iomuxCfg(&cfg);
}

#elif defined(__CPU_GR740)

static void console_cguClkEnable(void)
{
	_gr740_cguClkEnable(UART_CONSOLE_CGU);
}


static int console_cguClkStatus(void)
{
	return _gr740_cguClkStatus(UART_CONSOLE_CGU);
}


static void console_iomuxCfg(void)
{
	iomux_cfg_t cfg;

	cfg.opt = iomux_alternateio;
	cfg.pullup = 0;
	cfg.pulldn = 0;
	cfg.pin = UART_CONSOLE_TX;
	gaisler_iomuxCfg(&cfg);

	cfg.pin = UART_CONSOLE_RX;
	gaisler_iomuxCfg(&cfg);
}

#else

static void console_cguClkEnable(void)
{
}


static int console_cguClkStatus(void)
{
	return 1;
}


static void console_iomuxCfg(void)
{
}

#endif


void hal_consoleHook(void (*writeHook)(int, const void *, size_t))
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
		halconsole_common.writeHook(0, s, ptr - s);
	}
}


static u32 console_calcScaler(u32 baud)
{
	u32 scaler = (SYSCLK_FREQ / (baud * 8 + 7));
	return scaler;
}


void console_init(void)
{
	console_iomuxCfg();

	if (console_cguClkStatus() == 0) {
		console_cguClkEnable();
	}

	halconsole_common.uart = UART_CONSOLE_BASE;
	*(halconsole_common.uart + uart_scaler) = console_calcScaler(UART_BAUDRATE);
	hal_cpuDataStoreBarrier();
	*(halconsole_common.uart + uart_ctrl) = TX_EN;
}
