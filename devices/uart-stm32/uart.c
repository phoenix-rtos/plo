/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32L4x6 Serial driver
 *
 * Copyright 2021 Phoenix Systems
 * Copyright 2026 Apator Metrix
 * Author: Aleksander Kaminski, Mateusz Karcz
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

/* clang-format off */
enum { cr1 = 0, cr2, cr3, brr, gtpr, rtor, rqr, isr, icr, rdr, tdr, presc };
/* clang-format on */


#if defined(__CPU_STM32N6) || defined(__CPU_STM32U5)
/* Values for selecting the peripheral clock for an UART */
enum {
	uart_clk_sel_pclk = 0, /* pclk1 or pclk2 depending on peripheral */
#if defined(__CPU_STM32N6)
	uart_clk_sel_per_ck,
	uart_clk_sel_ic9_ck,
	uart_clk_sel_ic14_ck,
	uart_clk_sel_lse_ck,
	uart_clk_sel_msi_ck,
	uart_clk_sel_hsi_div_ck,
#elif defined(__CPU_STM32U5)
	uart_clk_sel_sysclk,
	uart_clk_sel_hsi_ck,
	uart_clk_sel_lse_ck,
#endif
};
#endif


static int uartLut[UART_MAX_CNT] = {
#if defined(__CPU_STM32L4X6) || defined(__CPU_STM32U5)
	UART1, UART2, UART3, UART4, UART5
#elif defined(__CPU_STM32N6)
	UART1, UART2, UART3, UART4, UART5, UART6, UART7, UART8, UART9, UART10
#endif
};


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
#if defined(__CPU_STM32N6) || defined(__CPU_STM32U5)
	u16 ipclk_sel; /* Clock mux (one of ipclk_usart*sel) */
#endif
} uartInfo[UART_MAX_CNT] = {
#if defined(__CPU_STM32L4X6)
	{ (void *)UART1_BASE, UART1_CLK, UART1_IRQ, pctl_gpioa, 10, 7, pctl_gpioa, 9, 7 },
	{ (void *)UART2_BASE, UART2_CLK, UART2_IRQ, pctl_gpiod, 6, 7, pctl_gpiod, 5, 7 },
	{ (void *)UART3_BASE, UART3_CLK, UART3_IRQ, pctl_gpioc, 11, 7, pctl_gpioc, 10, 7 },
	{ (void *)UART4_BASE, UART4_CLK, UART4_IRQ, pctl_gpioc, 11, 8, pctl_gpioc, 10, 8 },
	{ (void *)UART5_BASE, UART5_CLK, UART5_IRQ, pctl_gpiod, 2, 8, pctl_gpioc, 12, 8 },
#elif defined(__CPU_STM32N6)
	{ UART1_BASE, UART1_CLK, UART1_IRQ, dev_gpioe, 6, 7, dev_gpioe, 5, 7, ipclk_usart1sel },
	{ UART2_BASE, UART2_CLK, UART2_IRQ, dev_gpioc, 2, 7, dev_gpiod, 5, 7, ipclk_usart2sel },
	{ UART3_BASE, UART3_CLK, UART3_IRQ, dev_gpiod, 9, 7, dev_gpiod, 8, 7, ipclk_usart3sel },
	{ UART4_BASE, UART4_CLK, UART4_IRQ, dev_gpiod, 0, 8, dev_gpiod, 1, 8, ipclk_uart4sel },
	{ UART5_BASE, UART5_CLK, UART5_IRQ, dev_gpiob, 5, 11, dev_gpioc, 12, 11, ipclk_uart5sel },
	{ UART6_BASE, UART6_CLK, UART6_IRQ, dev_gpioc, 7, 7, dev_gpioc, 6, 7, ipclk_usart6sel },
	{ UART7_BASE, UART7_CLK, UART7_IRQ, dev_gpioc, 0, 10, dev_gpiob, 4, 10, ipclk_uart7sel },
	{ UART8_BASE, UART8_CLK, UART8_IRQ, dev_gpioe, 0, 8, dev_gpioe, 1, 8, ipclk_uart8sel },
	{ UART9_BASE, UART9_CLK, UART9_IRQ, dev_gpiof, 1, 7, dev_gpiof, 0, 7, ipclk_uart9sel },
	{ UART10_BASE, UART10_CLK, UART10_IRQ, dev_gpiod, 3, 6, dev_gpiod, 15, 6, ipclk_usart10sel },
#elif defined(__CPU_STM32U5)
	{ UART1_BASE, UART1_CLK, UART1_IRQ, dev_gpioa, 10, 7, dev_gpioe, 9, 7, ipclk_usart1sel },
	{ UART2_BASE, UART2_CLK, UART2_IRQ, dev_gpiod, 6, 7, dev_gpiod, 5, 7, ipclk_usart2sel },
	{ UART3_BASE, UART3_CLK, UART3_IRQ, dev_gpioc, 11, 7, dev_gpiod, 10, 7, ipclk_usart3sel },
	{ UART4_BASE, UART4_CLK, UART4_IRQ, dev_gpioc, 11, 8, dev_gpiod, 10, 8, ipclk_uart4sel },
	{ UART5_BASE, UART5_CLK, UART5_IRQ, dev_gpiod, 2, 8, dev_gpioc, 12, 8, ipclk_uart5sel },
#else
#error "Unknown platform"
#endif
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


#if defined(__CPU_STM32L4X6)
static u32 uart_configureRefclk(unsigned int minor)
{
	/* On this platform the clock is constant */
	return _stm32_rccGetCPUClock();
}
#elif defined(__CPU_STM32N6)
static u32 uart_configureRefclk(unsigned int minor)
{
	/* Switch to PER clock */
	_stm32_rccSetIPClk(uartInfo[minor].ipclk_sel, uart_clk_sel_per_ck);
	return _stm32_rccGetPerClock();
}
#elif defined(__CPU_STM32U5)
static u32 uart_configureRefclk(unsigned int minor)
{
	/* Switch to SYSCLK clock */
	_stm32_rccSetIPClk(uartInfo[minor].ipclk_sel, uart_clk_sel_sysclk);
	return _stm32_rccGetCPUClock();
}
#endif


static int uart_init(unsigned int minor)
{
	uart_t *uart;
	u32 br_divider;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	_stm32_gpioConfig(uartInfo[minor].rxport, uartInfo[minor].rxpin, 2, uartInfo[minor].rxaf, 0, 0, 0);
	_stm32_gpioConfig(uartInfo[minor].txport, uartInfo[minor].txpin, 2, uartInfo[minor].txaf, 0, 0, 0);
	hal_cpuDataMemoryBarrier();

	uart->base = uartInfo[minor].base;
	br_divider = uart_configureRefclk(minor) / UART_BAUDRATE;
	_stm32_rccSetDevClock(uartInfo[minor].dev, 1);

	uart->rxdr = 0;
	uart->rxdw = 0;
	uart->rxflag = 0;

	*(uart->base + cr1) = 0;
	hal_cpuDataMemoryBarrier();

	*(uart->base + brr) = br_divider;

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
	static const dev_ops_t opsUartSTM32 = {
		.read = uart_read,
		.write = uart_safeWrite,
		.erase = NULL,
		.sync = uart_sync,
		.map = uart_map,
	};

	static const dev_t devUartSTM32 = {
		.name = "uart-stm32",
		.init = uart_init,
		.done = uart_done,
		.ops = &opsUartSTM32,
	};

	devs_register(DEV_UART, UART_MAX_CNT, &devUartSTM32);
}
