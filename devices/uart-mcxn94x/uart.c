/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * MCX N94x Serial driver
 *
 * Copyright 2020, 2024 Phoenix Systems
 * Author: Hubert Buczyński, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <board_config.h>

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>

#define BUFFER_SIZE 0x200

typedef struct {
	volatile u32 *base;
	unsigned int irq;

	u16 rxFifoSz;
	u16 txFifoSz;

	u8 rxBuff[BUFFER_SIZE];
	volatile u16 rxHead;
	volatile u16 rxTail;

	u8 txBuff[BUFFER_SIZE];
	volatile u16 txHead;
	volatile u16 txTail;
	volatile u8 tFull;
} uart_t;


static struct {
	uart_t uarts[UART_MAX_CNT];
	int init;
} uart_common;


/* clang-format off */
enum { veridr = 0, paramr, globalr, pincfgr, baudr, statr, ctrlr, datar, matchr, modirr, fifor, waterr };
/* clang-format on */


static const u32 fifoSzLut[] = { 1, 4, 8, 16, 32, 64, 128, 256 };


/* clang-format off */
const int uartLut[] = {
	(FLEXCOMM0_SEL == FLEXCOMM_UART) || (FLEXCOMM0_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM1_SEL == FLEXCOMM_UART) || (FLEXCOMM1_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM2_SEL == FLEXCOMM_UART) || (FLEXCOMM2_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM3_SEL == FLEXCOMM_UART) || (FLEXCOMM3_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM4_SEL == FLEXCOMM_UART) || (FLEXCOMM4_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM5_SEL == FLEXCOMM_UART) || (FLEXCOMM5_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM6_SEL == FLEXCOMM_UART) || (FLEXCOMM6_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM7_SEL == FLEXCOMM_UART) || (FLEXCOMM7_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM8_SEL == FLEXCOMM_UART) || (FLEXCOMM8_SEL == FLEXCOMM_UARTI2C),
	(FLEXCOMM9_SEL == FLEXCOMM_UART) || (FLEXCOMM9_SEL == FLEXCOMM_UARTI2C)
};


static const struct {
	volatile u32 *base;
	int tx;
	int rx;
	int txalt;
	int rxalt;
	int irq;
} info[10] = {
	{ .base = FLEXCOMM0_BASE, .tx = UART0_TX_PIN, .rx = UART0_RX_PIN, .txalt = UART0_TX_ALT, .rxalt = UART0_RX_ALT, .irq = FLEXCOMM0_IRQ },
	{ .base = FLEXCOMM1_BASE, .tx = UART1_TX_PIN, .rx = UART1_RX_PIN, .txalt = UART1_TX_ALT, .rxalt = UART1_RX_ALT, .irq = FLEXCOMM1_IRQ },
	{ .base = FLEXCOMM2_BASE, .tx = UART2_TX_PIN, .rx = UART2_RX_PIN, .txalt = UART2_TX_ALT, .rxalt = UART2_RX_ALT, .irq = FLEXCOMM2_IRQ },
	{ .base = FLEXCOMM3_BASE, .tx = UART3_TX_PIN, .rx = UART3_RX_PIN, .txalt = UART3_TX_ALT, .rxalt = UART3_RX_ALT, .irq = FLEXCOMM3_IRQ },
	{ .base = FLEXCOMM4_BASE, .tx = UART4_TX_PIN, .rx = UART4_RX_PIN, .txalt = UART4_TX_ALT, .rxalt = UART4_RX_ALT, .irq = FLEXCOMM4_IRQ },
	{ .base = FLEXCOMM5_BASE, .tx = UART5_TX_PIN, .rx = UART5_RX_PIN, .txalt = UART5_TX_ALT, .rxalt = UART5_RX_ALT, .irq = FLEXCOMM5_IRQ },
	{ .base = FLEXCOMM6_BASE, .tx = UART6_TX_PIN, .rx = UART6_RX_PIN, .txalt = UART6_TX_ALT, .rxalt = UART6_RX_ALT, .irq = FLEXCOMM6_IRQ },
	{ .base = FLEXCOMM7_BASE, .tx = UART7_TX_PIN, .rx = UART7_RX_PIN, .txalt = UART7_TX_ALT, .rxalt = UART7_RX_ALT, .irq = FLEXCOMM7_IRQ },
	{ .base = FLEXCOMM8_BASE, .tx = UART8_TX_PIN, .rx = UART8_RX_PIN, .txalt = UART8_TX_ALT, .rxalt = UART8_RX_ALT, .irq = FLEXCOMM8_IRQ },
	{ .base = FLEXCOMM9_BASE, .tx = UART9_TX_PIN, .rx = UART9_RX_PIN, .txalt = UART9_TX_ALT, .rxalt = UART9_RX_ALT, .irq = FLEXCOMM9_IRQ },
};
/* clang-format on */


/* TODO: temporary solution, it should be moved to device tree */
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


static int uart_getRXcount(uart_t *uart)
{
	return (*(uart->base + waterr) >> 24) & 0xff;
}


static int uart_getTXcount(uart_t *uart)
{
	return (*(uart->base + waterr) >> 8) & 0xff;
}


static int uart_handleIntr(unsigned int irq, void *buff)
{
	uart_t *uart = (uart_t *)buff;

	if (uart == NULL) {
		return 0;
	}

	/* Receive */
	while (uart_getRXcount(uart) != 0) {
		uart->rxBuff[uart->rxHead] = *(uart->base + datar);
		uart->rxHead = (uart->rxHead + 1) % BUFFER_SIZE;
		if (uart->rxHead == uart->rxTail) {
			uart->rxTail = (uart->rxTail + 1) % BUFFER_SIZE;
		}
	}

	/* Transmit */
	while (uart_getTXcount(uart) < uart->txFifoSz) {
		if ((uart->txHead + 1) % BUFFER_SIZE != uart->txTail) {
			uart->txHead = (uart->txHead + 1) % BUFFER_SIZE;
			*(uart->base + datar) = uart->txBuff[uart->txHead];
			uart->tFull = 0;
		}
		else {
			*(uart->base + ctrlr) &= ~(1 << 23);
			break;
		}
	}

	return 0;
}


static u32 calculate_baudrate(int baud)
{
	int osr, sbr, bestSbr = 0, bestOsr = 0, bestErr = 1000, t;

	if (baud == 0) {
		return 0;
	}

	for (osr = 3; osr < 32; ++osr) {
		sbr = UART_CLK / (baud * (osr + 1));
		sbr &= 0xfff;
		t = UART_CLK / (sbr * (osr + 1));

		if (t > baud) {
			t = ((t - baud) * 1000) / baud;
		}
		else {
			t = ((baud - t) * 1000) / baud;
		}

		if (t < bestErr) {
			bestErr = t;
			bestOsr = osr;
			bestSbr = sbr;
		}

		/* Finish if error is < 1% */
		if (bestErr < 10) {
			break;
		}
	}

	return (bestOsr << 24) | ((bestOsr <= 6) << 17) | bestSbr;
}


static void uart_initPins(unsigned int minor)
{
	_mcxn94x_portPinConfig(info[minor].rx, info[minor].rxalt, MCX_PIN_SLOW | MCX_PIN_WEAK | MCX_PIN_PULLUP_WEAK | MCX_PIN_INPUT_BUFFER_ENABLE);
	_mcxn94x_portPinConfig(info[minor].tx, info[minor].txalt, MCX_PIN_SLOW | MCX_PIN_WEAK);
}


/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	uart_t *uart;
	size_t l, cnt;
	time_t start;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	start = hal_timerGet();
	while (uart->rxHead == uart->rxTail) {
		if ((hal_timerGet() - start) >= timeout) {
			return -ETIME;
		}
	}
	hal_interruptsDisable(info[minor].irq);

	if (uart->rxHead > uart->rxTail) {
		l = min(uart->rxHead - uart->rxTail, len);
	}
	else {
		l = min(BUFFER_SIZE - uart->rxTail, len);
	}

	hal_memcpy(buff, &uart->rxBuff[uart->rxTail], l);
	cnt = l;
	if ((len > l) && (uart->rxHead < uart->rxTail)) {
		hal_memcpy((char *)buff + l, &uart->rxBuff[0], min(len - l, uart->rxHead));
		cnt += min(len - l, uart->rxHead);
	}
	uart->rxTail = ((uart->rxTail + cnt) % BUFFER_SIZE);

	hal_interruptsEnable(info[minor].irq);

	return cnt;
}


static ssize_t uart_write(unsigned int minor, const void *buff, size_t len)
{
	uart_t *uart;
	size_t l, cnt = 0;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	while (uart->txHead == uart->txTail && uart->tFull) {
	}

	hal_interruptsDisable(info[minor].irq);
	if (uart->txHead > uart->txTail) {
		l = min(uart->txHead - uart->txTail, len);
	}
	else {
		l = min(BUFFER_SIZE - uart->txTail, len);
	}

	hal_memcpy(&uart->txBuff[uart->txTail], buff, l);
	cnt = l;
	if ((len > l) && (uart->txTail >= uart->txHead)) {
		hal_memcpy(uart->txBuff, (const char *)buff + l, min(len - l, uart->txHead));
		cnt += min(len - l, uart->txHead);
	}

	/* Initialize sending */
	if (uart->txTail == uart->txHead) {
		*(uart->base + datar) = uart->txBuff[uart->txHead];
	}

	uart->txTail = ((uart->txTail + cnt) % BUFFER_SIZE);

	if (uart->txTail == uart->txHead) {
		uart->tFull = 1;
	}

	*(uart->base + ctrlr) |= 1 << 23;

	hal_interruptsEnable(info[minor].irq);

	return cnt;
}


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	size_t l;

	for (l = 0; l < len; l += res) {
		res = uart_write(minor, (const char *)buff + l, len - l);
		if (res < 0) {
			return -ENXIO;
		}
	}

	return len;
}


static int uart_sync(unsigned int minor)
{
	uart_t *uart;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	/* Wait for transmission activity complete */
	while ((*(uart->base + statr) & (1 << 22)) == 0) {
	}

	return EOK;
}


static int uart_done(unsigned int minor)
{
	uart_t *uart;

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	uart_sync(minor);

	/* disable TX and RX */
	*(uart->base + ctrlr) &= ~((1 << 19) | (1 << 18));
	*(uart->base + ctrlr) &= ~((1 << 23) | (1 << 21));

	hal_interruptsSet(uart->irq, NULL, NULL);

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


static int uart_init(unsigned int minor)
{
	u32 t;
	uart_t *uart;
	static const int baudrate[10] = { UART0_BAUDRATE, UART1_BAUDRATE, UART2_BAUDRATE,
		UART3_BAUDRATE, UART4_BAUDRATE, UART5_BAUDRATE, UART6_BAUDRATE, UART7_BAUDRATE,
		UART8_BAUDRATE, UART9_BAUDRATE };

	uart = uart_getInstance(minor);
	if (uart == NULL) {
		return -EINVAL;
	}

	if (uart_common.init == 0) {
		uart_initPins(minor);
		uart_common.init = 1;
	}

	uart->base = info[minor].base;

	/* Disable TX and RX */
	*(uart->base + ctrlr) &= ~((1 << 19) | (1 << 18));

	/* Reset all internal logic and registers, except the Global Register */
	*(uart->base + globalr) |= 1 << 1;
	hal_cpuDataMemoryBarrier();
	*(uart->base + globalr) &= ~(1 << 1);
	hal_cpuDataMemoryBarrier();

	/* Disable input trigger */
	*(uart->base + pincfgr) &= ~3;

	/* Set 115200 default baudrate */
	t = *(uart->base + baudr) & ~((0x1f << 24) | (1 << 17) | 0xfff);
	*(uart->base + baudr) = t | calculate_baudrate(baudrate[UART_CONSOLE]);

	/* Set 8 bit and no parity mode */
	*(uart->base + ctrlr) &= ~0x117;

	/* One stop bit */
	*(uart->base + baudr) &= ~(1 << 13);

	*(uart->base + waterr) = 0;

	/* Enable FIFO */
	*(uart->base + fifor) |= (1 << 7) | (1 << 3);
	*(uart->base + fifor) |= 0x3 << 14;

	/* Clear all status flags */
	*(uart->base + statr) |= 0xc01fc000;

	uart->rxFifoSz = fifoSzLut[*(uart->base + fifor) & 0x7];
	uart->txFifoSz = fifoSzLut[(*(uart->base + fifor) >> 4) & 0x7];

	/* Enable receiver interrupt */
	*(uart->base + ctrlr) |= 1 << 21;

	/* Enable TX and RX */
	*(uart->base + ctrlr) |= (1 << 19) | (1 << 18);

	hal_interruptsSet(info[minor].irq, uart_handleIntr, (void *)uart);

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
