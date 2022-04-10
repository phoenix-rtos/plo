/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * nRF9160 UART driver
 *
 * Copyright 2022 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <hal/hal.h>
#include <lib/errno.h>
#include <lib/lib.h>
#include <devices/devs.h>

#include <board_config.h>


/* max value for uarte_txd_maxcnt register */
#define TX_DMA_SIZE_MAX 8191
#define BUFFER_SIZE     0x20


typedef struct {
	volatile unsigned int *base;
	unsigned int cnt;
	unsigned int irq;

	volatile char *tx_dma;
	volatile char *rx_dma;

	u8 dataRx[BUFFER_SIZE];
	cbuffer_t cbuffRx;

	u8 dataTx[BUFFER_SIZE];
	cbuffer_t cbuffTx;
} uart_t;


struct {
	uart_t uarts[UART_MAX_CNT];
} uart_common;

/* clang-format off */
enum { uarte_startrx = 0, uarte_stoprx, uarte_starttx, uarte_stoptx, uarte_flushrx = 11,
	uarte_events_cts = 64, uarte_events_rxdrdy = 66, uarte_events_endrx = 68, uarte_events_txdrdy = 71,
	uarte_events_endtx, uarte_events_error, uarte_events_rxto = 81, uarte_events_txstarted = 84,
	uarte_inten = 192, uarte_intenset, uarte_intenclr, uarte_errorsrc = 288, uarte_enable = 320,
	uarte_psel_rts = 322, uarte_psel_txd, uarte_psel_cts, uarte_psel_rxd, uarte_baudrate = 329,
	uarte_rxd_ptr = 333, uarte_rxd_maxcnt, uarte_rxd_amount, uarte_txd_ptr = 337, uarte_txd_maxcnt, uarte_txd_amount,
	uarte_config = 347 };
/* clang-format on */

static int uartLut[] = { UART0, UART1, UART2, UART3 };


static const struct {
	volatile unsigned int *base;
	unsigned int irq;
	unsigned int baudrate;
	volatile char *tx_dma;
	volatile char *rx_dma;
	unsigned char txpin;
	unsigned char rxpin;
	unsigned char rtspin;
	unsigned char ctspin;
} uartInfo[] = {
	{ (void *)UART0_BASE, UART0_IRQ, UART_BAUDRATE, (void *)UART0_TX_DMA, (void *)UART0_RX_DMA,
		UART0_TX, UART0_RX, UART0_RTS, UART0_CTS },
	{ (void *)UART1_BASE, UART1_IRQ, UART_BAUDRATE, (void *)UART1_TX_DMA, (void *)UART1_RX_DMA,
		UART1_TX, UART1_RX, UART1_RTS, UART1_CTS },
	{ (void *)UART2_BASE, UART2_IRQ, UART_BAUDRATE, (void *)UART2_TX_DMA, (void *)UART2_RX_DMA,
		UART2_TX, UART2_RX, UART2_RTS, UART2_CTS },
	{ (void *)UART3_BASE, UART3_IRQ, UART_BAUDRATE, (void *)UART3_TX_DMA, (void *)UART3_RX_DMA,
		UART3_TX, UART3_RX, UART3_RTS, UART3_CTS }
};


static uart_t *uart_getInstance(unsigned int minor)
{
	if (minor >= UART_MAX_CNT) {
		return NULL;
	}

	if (uartLut[minor] == 0) {
		return NULL;
	}

	return &uart_common.uarts[minor];
}


static int uart_handleIntr(unsigned int irq, void *buff)
{
	uart_t *uart = (uart_t *)buff;
	char temp;

	if (uart == NULL) {
		return -EINVAL;
	}

	if (*(uart->base + uarte_events_rxdrdy)) {
		/* clear rxdrdy event flag */
		*(uart->base + uarte_events_rxdrdy) = 0u;

		if (uart->cnt < UART_RX_DMA_SIZE) {
			temp = *(uart->rx_dma + uart->cnt);
			lib_cbufWrite(&uart->cbuffRx, &temp, 1);
			uart->cnt++;
		}
	}
	if (*(uart->base + uarte_events_endrx)) {
		/* clear endrx event flag */
		*(uart->base + uarte_events_endrx) = 0u;
		uart->cnt = 0;
		*(uart->base + uarte_startrx) = 1u;
		hal_cpuDataMemoryBarrier();
	}

	return 0;
}

/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	size_t res;
	uart_t *uart;
	time_t start;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	start = hal_timerGet();
	while (lib_cbufEmpty(&uart->cbuffRx)) {
		if ((hal_timerGet() - start) >= timeout) {
			return -ETIME;
		}
	}

	hal_interruptsDisable(uartInfo[minor].irq);
	res = lib_cbufRead(&uart->cbuffRx, buff, len);
	hal_interruptsEnable(uartInfo[minor].irq);

	return res;
}


static size_t uart_send(uart_t *uart, size_t len)
{
	*(uart->base + uarte_txd_maxcnt) = len;
	*(uart->base + uarte_starttx) = 1u;
	while (*(uart->base + uarte_events_txstarted) != 1u) {
		;
	}

	*(uart->base + uarte_events_txstarted) = 0u;

	while (*(uart->base + uarte_events_endtx) != 1u) {
		;
	}

	*(uart->base + uarte_events_endtx) = 0u;

	return (size_t)(*(uart->base + uarte_txd_amount));
}


static ssize_t uart_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	uart_t *uart;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	if (len > TX_DMA_SIZE_MAX) {
		return -EIO;
	}

	hal_memcpy((void *)uart->tx_dma, buff, len);

	return (ssize_t)uart_send(uart, len);
}


static int uart_sync(unsigned int minor)
{
	uart_t *uart;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	while (!lib_cbufEmpty(&uart->cbuffTx)) {
		;
	}

	/* endtx flag is asserted in uart_send so mustn't be checked here */

	return EOK;
}


static int uart_done(unsigned int minor)
{
	uart_t *uart;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	/* Wait for transmission activity complete */
	(void)uart_sync(minor);

	*(uart->base + uarte_stoptx) = 1u;
	*(uart->base + uarte_stoprx) = 1u;
	/* disable all uart interrupts */
	*(uart->base + uarte_intenclr) = 0xffffffffu;
	hal_cpuDataMemoryBarrier();
	*(uart->base + uarte_enable) = 0u;
	hal_cpuDataMemoryBarrier();

	hal_interruptsSet(uartInfo[minor].irq, NULL, NULL);

	return EOK;
}


static int uart_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	if (minor >= UART_MAX_CNT) {
		return -EINVAL;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* uart is not mappable to any region */
	return dev_isNotMappable;
}


/* Init pins according to nrf9160 product specification */
static int uart_configPins(unsigned int minor)
{
	int ret;

	ret = _nrf91_gpioConfig(uartInfo[minor].txpin, gpio_output, gpio_nopull);
	if (ret < 0) {
		return ret;
	}

	ret = _nrf91_gpioConfig(uartInfo[minor].rxpin, gpio_input, gpio_nopull);
	if (ret < 0) {
		return ret;
	}

	ret = _nrf91_gpioConfig(uartInfo[minor].rtspin, gpio_output, gpio_nopull);
	if (ret < 0) {
		return ret;
	}

	ret = _nrf91_gpioConfig(uartInfo[minor].ctspin, gpio_input, gpio_pulldown);
	if (ret < 0) {
		return ret;
	}

	ret = _nrf91_gpioSet(uartInfo[minor].txpin, gpio_high);
	if (ret < 0) {
		return ret;
	}

	ret = _nrf91_gpioSet(uartInfo[minor].rtspin, gpio_high);
	if (ret < 0) {
		return ret;
	}

	return EOK;
}


static int uart_init(unsigned int minor)
{
	int ret;
	uart_t *uart;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	uart->base = uartInfo[minor].base;
	uart->tx_dma = uartInfo[minor].tx_dma;
	uart->rx_dma = uartInfo[minor].rx_dma;

	/* Disable uarte instance - changing uarte config */
	if ((*(uart->base + uarte_enable) & 0x08) != 0) {
		*(uart->base + uarte_enable) = 0u;
		hal_cpuDataMemoryBarrier();
	}

	ret = uart_configPins(minor);
	if (ret < 0) {
		return ret;
	}

	/* Select pins */
	*(uart->base + uarte_psel_txd) = uartInfo[minor].txpin;
	*(uart->base + uarte_psel_rxd) = uartInfo[minor].rxpin;
	*(uart->base + uarte_psel_rts) = uartInfo[minor].rtspin;
	*(uart->base + uarte_psel_cts) = uartInfo[minor].ctspin;

	/* Currently supported baud rates - 115200, 9600
	   TODO: calculate uarte_baudrate register in run time */
	switch (uartInfo[minor].baudrate) {
		case 9600:
			*(uart->base + uarte_baudrate) = 0x00275000u;
			break;
		case 115200:
		default:
			*(uart->base + uarte_baudrate) = 0x01d60000u;
			break;
	}

	/* Default settings - hardware flow control disabled, exclude parity bit, one stop bit */
	*(uart->base + uarte_config) = 0u;
	/* Set default max number of bytes in specific buffers */
	*(uart->base + uarte_txd_maxcnt) = TX_DMA_SIZE_MAX;
	*(uart->base + uarte_rxd_maxcnt) = UART_RX_DMA_SIZE;

	/* Set default memory regions for uart dma */
	*(uart->base + uarte_txd_ptr) = (u32)uartInfo[minor].tx_dma;
	*(uart->base + uarte_rxd_ptr) = (u32)uartInfo[minor].rx_dma;

	/* Disable all uart interrupts */
	*(uart->base + uarte_intenclr) = 0xffffffffu;
	/* Enable rxdrdy and endrx interruts */
	*(uart->base + uarte_intenset) = 0x14u;
	hal_cpuDataMemoryBarrier();

	lib_cbufInit(&uart->cbuffTx, uart->dataTx, BUFFER_SIZE);
	lib_cbufInit(&uart->cbuffRx, uart->dataRx, BUFFER_SIZE);

	/* Enable uarte instance */
	*(uart->base + uarte_enable) = 0x8u;
	hal_cpuDataMemoryBarrier();
	uart->cnt = 0;
	*(uart->base + uarte_startrx) = 1u;
	hal_cpuDataMemoryBarrier();

	lib_printf("\ndev/uart: Initializing uart(%d.%d)", DEV_UART, minor);
	hal_interruptsSet(uartInfo[minor].irq, uart_handleIntr, (void *)uart);

	return EOK;
}


__attribute__((constructor)) static void uart_reg(void)
{
	static const dev_handler_t h = {
		.init = uart_init,
		.done = uart_done,
		.read = uart_read,
		.write = uart_write,
		.erase = NULL,
		.sync = uart_sync,
		.map = uart_map,
	};

	devs_register(DEV_UART, UART_MAX_CNT, &h);
}
