/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32L4x6 Serial driver
 *
 * Copyright 2021 Phoenix Systems
 * Author: Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include <lib/lib.h>
#include <devices/devs.h>


typedef struct {
	volatile unsigned int *base;
	volatile unsigned char rxdfifo[128];
	volatile unsigned int rxdr;
	volatile unsigned int rxdw;
	volatile int rxflag;
} uart_t;


struct {
	uart_t uarts[UART_MAX_CNT];
} uart_common;


enum { cr1 = 0, cr2, cr3, brr, gtpr, rtor, rqr, isr, icr, rdr, tdr };


static int uartLut[] = { UART1, UART2, UART3, UART4, UART5 };


static const struct {
	void *base;
	int dev;
	unsigned int irq;
	int rxport;
	unsigned char rxpin;
	unsigned char rxaf;
	int txport;
	unsigned char txpin;
	unsigned char txaf;
} uartInfo[] = {
	{ (void *)UART1_BASE, UART1_CLK, UART1_IRQ, pctl_gpioa, 10, 7, pctl_gpioa, 9, 7 },
	{ (void *)UART2_BASE, UART2_CLK, UART2_IRQ, pctl_gpiod, 6, 7, pctl_gpiod, 5, 7 },
	{ (void *)UART3_BASE, UART3_CLK, UART3_IRQ, pctl_gpioc, 11, 7, pctl_gpioc, 10, 7 },
	{ (void *)UART4_BASE, UART4_CLK, UART4_IRQ, pctl_gpioc, 11, 8, pctl_gpioc, 10, 8 },
	{ (void *)UART5_BASE, UART5_CLK, UART5_IRQ, pctl_gpiod, 2, 8, pctl_gpioc, 12, 8 },
};


static uart_t *uart_getInstance(unsigned int minor)
{
	if (minor >= UART_MAX_CNT)
		return NULL;

	if (uartLut[minor] == 0)
		return NULL;

	return &uart_common.uarts[minor];
}


static int uart_handleIntr(unsigned int irq, void *buff)
{
	uart_t *uart = (uart_t *)buff;

	if (*(uart->base + isr) & ((1 << 5) | (1 << 3))) {
		/* Clear overrun error bit */
		*(uart->base + icr) |= (1 << 3);

		/* Rxd buffer not empty */
		uart->rxdfifo[uart->rxdw++] = *(uart->base + rdr);
		uart->rxdw %= sizeof(uart->rxdfifo);

		if (uart->rxdr == uart->rxdw)
			uart->rxdr = (uart->rxdr + 1) % sizeof(uart->rxdfifo);

		uart->rxflag = 1;
	}

	return 0;
}

/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	uart_t *uart;
	size_t cnt;
	time_t start;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	if (len == 0)
		return 0;

	hal_interruptsDisable(uartInfo[minor].irq);
	for (cnt = 0; cnt < len; ++cnt) {
		uart->rxflag = 0;
		if (uart->rxdr == uart->rxdw) {
			hal_interruptsEnable(uartInfo[minor].irq);
			start = hal_timerGet();
			while (!uart->rxflag) {
				if ((hal_timerGet() - start) >= timeout)
					return -ETIME;
			}
			hal_interruptsDisable(uartInfo[minor].irq);
		}
		((unsigned char *)buff)[cnt] = uart->rxdfifo[uart->rxdr++];
		uart->rxdr %= sizeof(uart->rxdfifo);
	}
	hal_interruptsEnable(uartInfo[minor].irq);

	return (ssize_t)cnt;
}


static ssize_t uart_write(unsigned int minor, const void *buff, size_t len)
{
	uart_t *uart;
	size_t cnt;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	for (cnt = 0; cnt < len; ++cnt) {
		while (!(*(uart->base + isr) & 0x80))
			;

		*(uart->base + tdr) = ((const unsigned char *)buff)[cnt];
	}

	return (ssize_t)cnt;
}


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	return uart_write(minor, buff, len);
}


static int uart_sync(unsigned int minor)
{
	uart_t *uart;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	while (!(*(uart->base + isr) & 0x40))
		;

	return EOK;
}


static int uart_done(unsigned int minor)
{
	uart_t *uart;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	/* Wait for transmission activity complete */
	(void)uart_sync(minor);

	*(uart->base + cr1) = 0;
	hal_cpuDataMemoryBarrier();
	_stm32_rccSetDevClock(uartInfo[minor].dev, 0);
	hal_cpuDataMemoryBarrier();
	_stm32_gpioConfig(uartInfo[minor].rxport, uartInfo[minor].rxpin, 0, 0, 0, 0, 0);
	_stm32_gpioConfig(uartInfo[minor].txport, uartInfo[minor].txpin, 0, 0, 0, 0, 0);

	hal_interruptsSet(uartInfo[minor].irq, NULL, NULL);

	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor >= UART_MAX_CNT)
		return -EINVAL;

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	/* uart is not mappable to any region */
	return dev_isNotMappable;
}


static int uart_init(unsigned int minor)
{
	uart_t *uart;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	_stm32_gpioConfig(uartInfo[minor].rxport, uartInfo[minor].rxpin, 2, uartInfo[minor].rxaf, 0, 0, 0);
	_stm32_gpioConfig(uartInfo[minor].txport, uartInfo[minor].txpin, 2, uartInfo[minor].txaf, 0, 0, 0);
	hal_cpuDataMemoryBarrier();

	uart->base = uartInfo[minor].base;
	_stm32_rccSetDevClock(uartInfo[minor].dev, 1);

	uart->rxdr = 0;
	uart->rxdw = 0;
	uart->rxflag = 0;

	*(uart->base + cr1) = 0;
	hal_cpuDataMemoryBarrier();

	*(uart->base + brr) = _stm32_rccGetCPUClock() / UART_BAUDRATE;

	*(uart->base + icr) = -1;
	(void)*(uart->base + rdr);

	*(uart->base + cr1) |= (1 << 5) | (1 << 3) | (1 << 2);
	hal_cpuDataMemoryBarrier();
	*(uart->base + cr1) |= 1;

	hal_cpuDataMemoryBarrier();

	hal_interruptsSet(uartInfo[minor].irq, uart_handleIntr, (void *)uart);

	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_handler_t h = {
		.init = uart_init,
		.done = uart_done,
		.read = uart_read,
		.write = uart_safeWrite,
		.erase = NULL,
		.sync = uart_sync,
		.map = uart_map,
	};

	devs_register(DEV_UART, UART_MAX_CNT, &h);
}
