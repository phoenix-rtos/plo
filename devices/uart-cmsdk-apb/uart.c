/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * ARM CMSDK APBUART Serial driver
 *
 * Copyright 2024 Phoenix Systems
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

#define UART_ACTIVE_CNT (UART0_ACTIVE + UART1_ACTIVE + UART2_ACTIVE + UART3_ACTIVE + UART4_ACTIVE + UART5_ACTIVE)

/* UART state bits */
#define TX_BUF_FULL (1 << 0)
#define RX_BUF_FULL (1 << 1)

/* UART control bits */
#define TX_EN     (1 << 0)
#define RX_EN     (1 << 1)
#define TX_INT_EN (1 << 2)
#define RX_INT_EN (1 << 3)

/* UART intstatus bits */
#define TX_INT (1 << 0)
#define RX_INT (1 << 1)


/* UART registers */
/* clang-format off */
enum { data = 0, state, ctrl, intstatus, bauddiv };
/* clang-format on */


typedef struct {
	vu32 *const base;
	const unsigned int rxirq;
	const unsigned int active;

	cbuffer_t cbuffRx;
} uart_t;


static struct {
	u8 dataRx[UART_ACTIVE_CNT][BUFFER_SIZE];
	uart_t uarts[UART_MAX_CNT];
} uart_common = {
	.uarts = {
		{ .base = UART0_BASE, .rxirq = UART0_RX_IRQ, .active = UART0_ACTIVE },
		{ .base = UART1_BASE, .rxirq = UART1_RX_IRQ, .active = UART1_ACTIVE },
		{ .base = UART2_BASE, .rxirq = UART2_RX_IRQ, .active = UART2_ACTIVE },
		{ .base = UART3_BASE, .rxirq = UART3_RX_IRQ, .active = UART3_ACTIVE },
		{ .base = UART4_BASE, .rxirq = UART4_RX_IRQ, .active = UART4_ACTIVE },
		{ .base = UART5_BASE, .rxirq = UART5_RX_IRQ, .active = UART5_ACTIVE },
	}
};


static inline void uart_rxData(uart_t *uart)
{
	u8 c;

	while ((*(uart->base + state) & RX_BUF_FULL) != 0) {
		c = *(uart->base + data) & 0xff;
		lib_cbufWrite(&uart->cbuffRx, &c, 1);
	}
	*(uart->base + intstatus) = RX_INT;
}


static inline void uart_txData(uart_t *uart, const void *buff, size_t len)
{
	size_t i;
	const u8 *c = buff;

	for (i = 0; i < len; i++) {
		/* No hardware FIFO, wait until TX buffer is empty */
		while ((*(uart->base + state) & TX_BUF_FULL) != 0) {
		}
		*(uart->base + data) = c[i];
	}
}


static int uart_irqHandler(unsigned int n, void *data)
{
	uart_t *uart = (uart_t *)data;
	u32 status = *(uart->base + state);

	if ((status & RX_BUF_FULL) != 0) {
		uart_rxData(uart);
	}

	return 0;
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

	if (uart->active == 0) {
		return -ENOSYS;
	}

	start = hal_timerGet();
	while (lib_cbufEmpty(&uart->cbuffRx) != 0) {
		if (hal_timerGet() - start > timeout) {
			return -ETIME;
		}
		hal_cpuHalt();
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

	if (uart->active == 0) {
		return -ENOSYS;
	}

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

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	if (uart->active == 0) {
		return -ENOSYS;
	}

	/* Wait until Tx shift register is empty */
	while ((*(uart->base + state) & TX_BUF_FULL) != 0) { }

	return EOK;
}


static int uart_done(unsigned int minor)
{
	uart_t *uart;

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	if (uart->active == 0) {
		return -ENOSYS;
	}

	(void)uart_sync(minor);

	*(uart->base + ctrl) = 0;

	hal_interruptsSet(uart->rxirq, NULL, NULL);

	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	if (uart_common.uarts[minor].active == 0) {
		return -ENOSYS;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}
	/* UART is not mappable to any region */
	return dev_isNotMappable;
}


static int uart_getActiveIdx(int minor)
{
	int i, idx = 0;

	for (i = 0; i < minor; i++) {
		if (uart_common.uarts[i].active != 0) {
			idx++;
		}
	}

	return idx;
}


static int uart_init(unsigned int minor)
{
	uart_t *uart;
	void *buf;

	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	uart = &uart_common.uarts[minor];

	if (uart->active == 0) {
		return -ENOSYS;
	}

	buf = uart_common.dataRx[uart_getActiveIdx(minor)];

	lib_cbufInit(&uart->cbuffRx, buf, BUFFER_SIZE);

	*(uart->base + bauddiv) = SYSCLK_FREQ / UART_BAUDRATE;
	hal_cpuDataSyncBarrier();

	hal_interruptsSet(uart->rxirq, uart_irqHandler, (void *)uart);

	/* Enable UART */
	*(uart->base + ctrl) = TX_EN | RX_EN | RX_INT_EN;

	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_ops_t opsUartCmsdkApb = {
		.read = uart_read,
		.write = uart_safeWrite,
		.erase = NULL,
		.sync = uart_sync,
		.map = uart_map,
	};

	static const dev_t devUartCmsdkApb = {
		.name = "uart-cmsdk-apb",
		.init = uart_init,
		.done = uart_done,
		.ops = &opsUartCmsdkApb,
	};

	devs_register(DEV_UART, UART_MAX_CNT, &devUartCmsdkApb);
}
