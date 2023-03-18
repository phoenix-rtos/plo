/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 Serial driver
 *
 * Copyright 2022 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>


#define BUFFER_SIZE 0x200

/* UART control bits */
#define RX_FIFO_INT (1 << 10)
#define TX_FIFO_INT (1 << 9)
#define PARITY_EN   (1 << 5)
#define TX_INT      (1 << 3)
#define RX_INT      (1 << 2)
#define TX_EN       (1 << 1)
#define RX_EN       (1 << 0)

/* UART status bits */
#define RX_FIFO_FULL  (1 << 10)
#define TX_FIFO_FULL  (1 << 9)
#define TX_FIFO_EMPTY (1 << 2)
#define TX_SR_EMPTY   (1 << 1)
#define DATA_READY    (1 << 0)


/* UART */
enum {
	uart_data,   /* Data register           : 0x00 */
	uart_status, /* Status register         : 0x04 */
	uart_ctrl,   /* Control register        : 0x08 */
	uart_scaler, /* Scaler reload register  : 0x0C */
	uart_dbg     /* FIFO debug register     : 0x10 */
};


typedef struct {
	volatile u32 *base;
	unsigned int irq;
	u16 clk;

	u8 dataRx[BUFFER_SIZE];
	cbuffer_t cbuffRx;
} uart_t;


static struct {
	uart_t uarts[UART_MAX_CNT];
} uart_common;


static const struct {
	vu32 *base;
	unsigned int irq;
	unsigned int txPin;
	unsigned int rxPin;
} info[UART_MAX_CNT] = {
	{ UART0_BASE, UART0_IRQ, UART0_TX, UART0_RX },
	{ UART1_BASE, UART1_IRQ, UART1_TX, UART1_RX },
	{ UART2_BASE, UART2_IRQ, UART2_TX, UART2_RX },
	{ UART3_BASE, UART3_IRQ, UART3_TX, UART3_RX },
	{ UART4_BASE, UART4_IRQ, UART4_TX, UART4_RX },
	{ UART5_BASE, UART5_IRQ, UART5_TX, UART5_RX }
};


static inline void uart_rxData(uart_t *uart)
{
	char c;
	/* Keep getting data until rx fifo is not empty */
	while ((*(uart->base + uart_status) & DATA_READY) != 0) {
		c = *(uart->base + uart_data) & 0xff;
		lib_cbufWrite(&uart->cbuffRx, &c, 1);
	}
}


static inline void uart_txData(uart_t *uart, const void *buff, size_t len)
{
	size_t i;
	const char *c = buff;

	for (i = 0; i < len; i++) {
		/* Fill until tx fifo is not full */
		while ((*(uart->base + uart_status) & TX_FIFO_FULL) != 0) {
		}
		*(uart->base + uart_data) = c[i];
	}
}


static int uart_irqHandler(unsigned int n, void *data)
{
	uart_t *uart = (uart_t *)data;
	u32 status = *(uart->base + uart_status);

	if ((status & DATA_READY) != 0) {
		uart_rxData(uart);
	}

	return 0;
}


static int uart_setPin(u8 pin)
{
	io_cfg_t uartCfg;
	uartCfg.opt = 0x1;
	uartCfg.pin = pin;
	uartCfg.pullup = 0;
	uartCfg.pulldn = 0;

	switch (pin) {
		case UART0_TX:
		case UART1_TX:
			/* pins used for SRAM */
			return -1;
		case UART2_TX:
		case UART3_TX:
		case UART4_TX:
		case UART5_TX:
			uartCfg.dir = GPIO_DIR_OUT;
			break;
		case UART0_RX:
		case UART1_RX:
			/* pins used for SRAM */
			return -1;
		case UART2_RX:
		case UART3_RX:
		case UART4_RX:
		case UART5_RX:
			uartCfg.dir = GPIO_DIR_IN;
			break;
		default:
			return -1;
	}

	return _gr716_ioCfg(&uartCfg);
}


/* From datasheet:
 * appropriate formula to calculate the scaler for desired baudrate,
 * using integer division where the remainder is discarded:
 * scaler = (sysclk_freq)/(baudrate * 8 + 7)
 */
static u32 uart_calcScaler(u32 baud)
{
	u32 scaler = (SYSCLK_FREQ / (baud * 8 + 7));

	return scaler;
}

/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	ssize_t res;
	uart_t *uart;
	time_t start;

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];
	start = hal_timerGet();
	while (lib_cbufEmpty(&uart->cbuffRx) != 0) {
		if (hal_timerGet() - start > timeout) {
			return -ETIME;
		}
	}
	hal_interruptsDisableAll();
	res = lib_cbufRead(&uart->cbuffRx, buff, len);
	hal_interruptsEnableAll();

	return res;
}


static ssize_t uart_write(unsigned int minor, const void *buff, size_t len)
{
	uart_t *uart;

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	uart_txData(uart, buff, len);

	return len;
}


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res = 0;
	size_t wrote = 0;

	while (wrote < len) {
		res = uart_write(minor, buff + wrote, len - wrote);
		if (res < 0) {
			return -ENXIO;
		}
		wrote += res;
	}

	return len;
}


static int uart_sync(unsigned int minor)
{
	uart_t *uart;

	if (minor == 0 || minor == 1) {
		return EOK;
	}

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	/* Wait until Tx shift register is empty */
	while ((*(uart->base + uart_status) & TX_SR_EMPTY) == 0) {
	}

	return EOK;
}


static int uart_done(unsigned int minor)
{
	int res;
	uart_t *uart;

	if (minor == 0 || minor == 1) {
		return EOK;
	}

	res = uart_sync(minor);

	if (res < 0) {
		return res;
	}
	uart = &uart_common.uarts[minor];

	/* Disable interrupts, TX & RX, FIFO */
	*(uart->base + uart_ctrl) = 0;
	*(uart->base + uart_scaler) = 0;
	hal_cpuDataStoreBarrier();

	_gr716_cguClkDisable(cgu_primary, cgudev_apbuart0 + minor);

	hal_interruptsSet(uart->irq, NULL, NULL);

	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	/* UART is not mappable to any region */
	int err = dev_isNotMappable;

	if (minor >= UART_MAX_CNT) {
		err = -EINVAL;
	}
	/* Device mode cannot be higher than map mode to copy data */
	else if ((mode & memmode) != mode) {
		err = -EINVAL;
	}
	return err;
}


static int uart_init(unsigned int minor)
{
	uart_t *uart;
	/* When running external SRAM, UART0 and UART1 pins cannot be used */
	if (minor == 0 || minor == 1) {
		return EOK;
	}

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	uart->base = info[minor].base;
	uart->irq = info[minor].irq;

	uart_done(minor);

	_gr716_cguClkEnable(cgu_primary, cgudev_apbuart0 + minor);

	uart_setPin(info[minor].txPin);
	uart_setPin(info[minor].rxPin);

	lib_cbufInit(&uart->cbuffRx, uart->dataRx, BUFFER_SIZE);

	*(uart->base + uart_scaler) = uart_calcScaler(UART_BAUDRATE);

	/* UART control - clear everything and: enable 1 stop bit,
	 * disable parity, enable RX interrupts, enable TX & RX */
	*(uart->base + uart_ctrl) = RX_INT | TX_EN | RX_EN;
	hal_cpuDataStoreBarrier();
	hal_interruptsSet(uart->irq, uart_irqHandler, (void *)uart);

	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_handler_t h = {
		.init = uart_init,
		.done = uart_done,
		.read = uart_read,
		.write = uart_safeWrite,
		.sync = uart_sync,
		.map = uart_map,
	};

	devs_register(DEV_UART, UART_MAX_CNT, &h);
}
