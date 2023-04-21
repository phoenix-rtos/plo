/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MXRT1064 Serial driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczyński
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <board_config.h>

#include <hal/hal.h>
#include <lib/lib.h>
#include <devices/devs.h>

#define CONCATENATE(x, y) x##y
#define PIN2MUX(x)        CONCATENATE(pctl_mux_gpio_, x)
#define PIN2PAD(x)        CONCATENATE(pctl_pad_gpio_, x)


#define BUFFER_SIZE 0x20

typedef struct {
	volatile u32 *base;
	unsigned int irq;

	u16 rxFifoSz;
	u16 txFifoSz;

	u8 dataRx[BUFFER_SIZE];
	cbuffer_t cbuffRx;

	u8 dataTx[BUFFER_SIZE];
	cbuffer_t cbuffTx;
} uart_t;


struct {
	uart_t uarts[UART_MAX_CNT];
	int clkInit;
} uart_common;


const u32 fifoSzLut[] = { 1, 4, 8, 16, 32, 64, 128, 256 };

const int uartLut[] = { UART1, UART2, UART3, UART4, UART5, UART6, UART7, UART8 };


/* TODO: specific uart information should be get from a device tree,
 *       using hardcoded defines is a temporary solution           */
const struct {
	volatile u32 *base;
	int dev;
	unsigned irq;
} info[] = {
	{ UART1_BASE, UART1_CLK, UART1_IRQ },
	{ UART2_BASE, UART2_CLK, UART2_IRQ },
	{ UART3_BASE, UART3_CLK, UART3_IRQ },
	{ UART4_BASE, UART4_CLK, UART4_IRQ },
	{ UART5_BASE, UART5_CLK, UART5_IRQ },
	{ UART6_BASE, UART6_CLK, UART6_IRQ },
	{ UART7_BASE, UART7_CLK, UART7_IRQ },
	{ UART8_BASE, UART8_CLK, UART8_IRQ }
};


enum { veridr = 0, paramr, globalr, pincfgr, baudr, statr, ctrlr, datar, matchr, modirr, fifor, waterr };


__attribute__((section(".noxip"))) static inline int uart_getRXcount(uart_t *uart)
{
	return (*(uart->base + waterr) >> 24) & 0xff;
}


__attribute__((section(".noxip"))) static inline int uart_getTXcount(uart_t *uart)
{
	return (*(uart->base + waterr) >> 8) & 0xff;
}


/* TODO: temporary solution, it should be moved to device tree */
static uart_t *uart_getInstance(unsigned int minor)
{
	if (minor >= UART_MAX_CNT)
		return NULL;

	if (uartLut[minor] == 0)
		return NULL;

	return &uart_common.uarts[minor];
}


__attribute__((section(".noxip"))) static int uart_handleIntr(unsigned int irq, void *buff)
{
	char c;
	u32 flags;
	uart_t *uart = (uart_t *)buff;

	if (uart == NULL)
		return 0;

	/* Error flags: parity, framing, noise, overrun */
	flags = *(uart->base + statr) & (0xf << 16);

	/* RX overrun: invalidate fifo */
	if (flags & (1 << 19))
		*(uart->base + fifor) |= 1 << 14;

	*(uart->base + statr) |= flags;

	/* Receive */
	while (uart_getRXcount(uart)) {
		c = *(uart->base + datar);
		lib_cbufWrite(&uart->cbuffRx, &c, 1);
	}

	/* Transmit */
	while (uart_getTXcount(uart) < uart->txFifoSz) {
		if (!lib_cbufEmpty(&uart->cbuffTx)) {
			lib_cbufRead(&uart->cbuffTx, &c, 1);
			*(uart->base + datar) = c;
		}
		else {
			*(uart->base + ctrlr) &= ~(1 << 23);
			break;
		}
	}

	return 0;
}


static int uart_muxVal(int mux)
{
	switch (mux) {
		case pctl_mux_gpio_b1_12:
		case pctl_mux_gpio_b1_13:
			return 1;

		case pctl_mux_gpio_b0_08:
		case pctl_mux_gpio_b0_09:
			return 3;

		case pctl_mux_gpio_sd_b1_00:
		case pctl_mux_gpio_sd_b1_01:
			return 4;
	}

	return 2;
}


static u32 calculate_baudrate(int baud)
{
	u32 osr, sbr, t, tDiff;
	u32 bestOsr = 0, bestSbr = 0, bestDiff = (u32)baud;

	if (baud <= 0)
		return 0;

	for (osr = 4; osr <= 32; ++osr) {
		/* find sbr value in range between 1 and 8191 */
		sbr = (UART_CLK / ((u32)baud * osr)) & 0x1fff;
		sbr = (sbr == 0) ? 1 : sbr;

		/* baud rate difference based on temporary osr and sbr */
		tDiff = UART_CLK / (osr * sbr) - (u32)baud;
		t = UART_CLK / (osr * (sbr + 1));

		/* select best values between sbr and sbr+1 */
		if (tDiff > (u32)baud - t) {
			tDiff = (u32)baud - t;
			sbr += (sbr < 0x1fff);
		}

		if (tDiff <= bestDiff) {
			bestDiff = tDiff;
			bestOsr = osr - 1;
			bestSbr = sbr;
		}
	}

	return (bestOsr << 24) | ((bestOsr <= 6) << 17) | (bestSbr & 0x1fff);
}


static int uart_getIsel(int mux, int *isel, int *val)
{
	switch (mux) {
		case pctl_mux_gpio_ad_b1_02:
			*isel = pctl_isel_lpuart2_tx;
			*val = 1;
			break;
		case pctl_mux_gpio_sd_b1_11:
			*isel = pctl_isel_lpuart2_tx;
			*val = 0;
			break;
		case pctl_mux_gpio_ad_b1_03:
			*isel = pctl_isel_lpuart2_rx;
			*val = 1;
			break;
		case pctl_mux_gpio_sd_b1_10:
			*isel = pctl_isel_lpuart2_rx;
			*val = 0;
			break;
		case pctl_mux_gpio_emc_13:
			*isel = pctl_isel_lpuart3_tx;
			*val = 1;
			break;
		case pctl_mux_gpio_ad_b1_06:
			*isel = pctl_isel_lpuart3_tx;
			*val = 0;
			break;
		case pctl_mux_gpio_b0_08:
			*isel = pctl_isel_lpuart3_tx;
			*val = 2;
			break;
		case pctl_mux_gpio_emc_14:
			*isel = pctl_isel_lpuart3_rx;
			*val = 1;
			break;
		case pctl_mux_gpio_ad_b1_07:
			*isel = pctl_isel_lpuart3_rx;
			*val = 0;
			break;
		case pctl_mux_gpio_b0_09:
			*isel = pctl_isel_lpuart3_rx;
			*val = 2;
			break;
		case pctl_mux_gpio_emc_15:
			*isel = pctl_isel_lpuart3_cts_b;
			*val = 0;
			break;
		case pctl_mux_gpio_ad_b1_04:
			*isel = pctl_isel_lpuart3_cts_b;
			*val = 1;
			break;
		case pctl_mux_gpio_emc_19:
			*isel = pctl_isel_lpuart4_tx;
			*val = 1;
			break;
		case pctl_mux_gpio_b1_00:
			*isel = pctl_isel_lpuart4_tx;
			*val = 2;
			break;
		case pctl_mux_gpio_sd_b1_00:
			*isel = pctl_isel_lpuart4_tx;
			*val = 0;
			break;
		case pctl_mux_gpio_emc_20:
			*isel = pctl_isel_lpuart4_rx;
			*val = 1;
			break;
		case pctl_mux_gpio_b1_01:
			*isel = pctl_isel_lpuart4_rx;
			*val = 2;
			break;
		case pctl_mux_gpio_sd_b1_01:
			*isel = pctl_isel_lpuart4_rx;
			*val = 0;
			break;
		case pctl_mux_gpio_emc_23:
			*isel = pctl_isel_lpuart5_tx;
			*val = 0;
			break;
		case pctl_mux_gpio_b1_12:
			*isel = pctl_isel_lpuart5_tx;
			*val = 1;
			break;
		case pctl_mux_gpio_emc_24:
			*isel = pctl_isel_lpuart5_rx;
			*val = 0;
			break;
		case pctl_mux_gpio_b1_13:
			*isel = pctl_isel_lpuart5_rx;
			*val = 1;
			break;
		case pctl_mux_gpio_emc_25:
			*isel = pctl_isel_lpuart6_tx;
			*val = 0;
			break;
		case pctl_mux_gpio_ad_b0_12:
			*isel = pctl_isel_lpuart6_tx;
			*val = 1;
			break;
		case pctl_mux_gpio_emc_26:
			*isel = pctl_isel_lpuart6_rx;
			*val = 0;
			break;
		case pctl_mux_gpio_ad_b0_03:
			*isel = pctl_isel_lpuart6_rx;
			*val = 1;
			break;
		case pctl_mux_gpio_emc_31:
			*isel = pctl_isel_lpuart7_tx;
			*val = 1;
			break;
		case pctl_mux_gpio_sd_b1_08:
			*isel = pctl_isel_lpuart7_tx;
			*val = 0;
			break;
		case pctl_mux_gpio_emc_32:
			*isel = pctl_isel_lpuart7_rx;
			*val = 1;
			break;
		case pctl_mux_gpio_sd_b1_09:
			*isel = pctl_isel_lpuart7_rx;
			*val = 0;
			break;
		case pctl_mux_gpio_emc_38:
			*isel = pctl_isel_lpuart8_tx;
			*val = 2;
			break;
		case pctl_mux_gpio_ad_b1_10:
			*isel = pctl_isel_lpuart8_tx;
			*val = 1;
			break;
		case pctl_mux_gpio_sd_b0_04:
			*isel = pctl_isel_lpuart8_tx;
			*val = 0;
			break;
		case pctl_mux_gpio_emc_39:
			*isel = pctl_isel_lpuart8_rx;
			*val = 2;
			break;
		case pctl_mux_gpio_ad_b1_11:
			*isel = pctl_isel_lpuart8_rx;
			*val = 1;
			break;
		case pctl_mux_gpio_sd_b0_05:
			*isel = pctl_isel_lpuart8_rx;
			*val = 0;
			break;
		default: return -1;
	}

	return 0;
}

/* TODO: temporary solution, it should be moved to device tree */
static void uart_initPins(void)
{
	int i, isel, val;
	static const int muxes[] = {
#if UART1
		PIN2MUX(UART1_TX_PIN),
		PIN2MUX(UART1_RX_PIN),
#endif
#if UART1_HW_FLOWCTRL
		PIN2MUX(UART1_RTS_PIN),
		PIN2MUX(UART1_CTS_PIN),
#endif
#if UART2
		PIN2MUX(UART2_TX_PIN),
		PIN2MUX(UART2_RX_PIN),
#endif
#if UART2_HW_FLOWCTRL
		PIN2MUX(UART2_RTS_PIN),
		PIN2MUX(UART2_CTS_PIN),
#endif
#if UART3
		PIN2MUX(UART3_TX_PIN),
		PIN2MUX(UART3_RX_PIN),
#endif
#if UART3_HW_FLOWCTRL
		PIN2MUX(UART3_RTS_PIN),
		PIN2MUX(UART3_CTS_PIN),
#endif
#if UART4
		PIN2MUX(UART4_TX_PIN),
		PIN2MUX(UART4_RX_PIN),
#endif
#if UART4_HW_FLOWCTRL
		PIN2MUX(UART4_RTS_PIN),
		PIN2MUX(UART4_CTS_PIN),
#endif
#if UART5
		PIN2MUX(UART5_TX_PIN),
		PIN2MUX(UART5_RX_PIN),
#endif
#if UART5_HW_FLOWCTRL
		PIN2MUX(UART5_RTS_PIN),
		PIN2MUX(UART5_CTS_PIN),
#endif
#if UART6
		PIN2MUX(UART6_TX_PIN),
		PIN2MUX(UART6_RX_PIN),
#endif
#if UART6_HW_FLOWCTRL
		PIN2MUX(UART6_RTS_PIN),
		PIN2MUX(UART6_CTS_PIN),
#endif
#if UART7
		PIN2MUX(UART7_TX_PIN),
		PIN2MUX(UART7_RX_PIN),
#endif
#if UART7_HW_FLOWCTRL
		PIN2MUX(UART7_RTS_PIN),
		PIN2MUX(UART7_CTS_PIN),
#endif
#if UART8
		PIN2MUX(UART8_TX_PIN),
		PIN2MUX(UART8_RX_PIN),
#endif
#if UART8_HW_FLOWCTRL
		PIN2MUX(UART8_RTS_PIN),
		PIN2MUX(UART8_CTS_PIN),
#endif
	};

	for (i = 0; i < sizeof(muxes) / sizeof(muxes[0]); ++i) {
		_imxrt_setIOmux(muxes[i], 0, uart_muxVal(muxes[i]));

		if (uart_getIsel(muxes[i], &isel, &val) < 0)
			continue;

		_imxrt_setIOisel(isel, val);
	}
}


/* Device interface */

static ssize_t uart_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	size_t res;
	uart_t *uart;
	time_t start;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	start = hal_timerGet();
	while (lib_cbufEmpty(&uart->cbuffRx)) {
		if ((hal_timerGet() - start) >= timeout)
			return -ETIME;
	}

	hal_interruptsDisable(info[minor].irq);
	res = lib_cbufRead(&uart->cbuffRx, buff, len);
	hal_interruptsEnable(info[minor].irq);

	return res;
}


static ssize_t uart_write(unsigned int minor, const void *buff, size_t len)
{
	size_t res;
	uart_t *uart;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	while (uart->cbuffTx.full)
		;

	hal_interruptsDisable(info[minor].irq);
	res = lib_cbufWrite(&uart->cbuffTx, buff, len);
	*(uart->base + ctrlr) |= 1 << 23;

	hal_interruptsEnable(info[minor].irq);

	return res;
}


static ssize_t uart_safeWrite(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	size_t l;

	for (l = 0; l < len; l += res) {
		if ((res = uart_write(minor, (const char *)buff + l, len - l)) < 0)
			return -ENXIO;
	}

	return len;
}


static int uart_sync(unsigned int minor)
{
	uart_t *uart;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	while (!lib_cbufEmpty(&uart->cbuffTx))
		;

	/* Wait for transmission activity complete */
	while ((*(uart->base + statr) & (1 << 22)) == 0)
		;

	return EOK;
}


static int uart_done(unsigned int minor)
{
	uart_t *uart;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	uart_sync(minor);

	/* Disable TX and RX */
	*(uart->base + ctrlr) &= ~((1 << 19) | (1 << 18));
	hal_cpuDataMemoryBarrier();

	/* Disable overrun, noise, framing error, TX and RX interrupts */
	*(uart->base + ctrlr) &= ~((1 << 27) | (1 << 26) | (1 << 25) | (1 << 23) | (1 << 21));

	/* Flush TX and RX fifo */
	*(uart->base + fifor) |= (1 << 15) | (1 << 14);

	/* Safely perform LPUART software reset procedure */
	*(uart->base + globalr) |= (1 << 1);
	hal_cpuDataMemoryBarrier();
	*(uart->base + globalr) &= ~(1 << 1);
	hal_cpuDataMemoryBarrier();

	hal_interruptsSet(uart->irq, NULL, NULL);

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
	u32 t;
	uart_t *uart;

	if ((uart = uart_getInstance(minor)) == NULL)
		return -EINVAL;

	if (uart_common.clkInit == 0) {
		_imxrt_ccmSetMux(clk_mux_uart, 0);
		_imxrt_ccmSetDiv(clk_div_uart, 0);
		uart_initPins();

		uart_common.clkInit = 1;
	}

	uart->base = info[minor].base;
	lib_cbufInit(&uart->cbuffTx, uart->dataTx, BUFFER_SIZE);
	lib_cbufInit(&uart->cbuffRx, uart->dataRx, BUFFER_SIZE);

	_imxrt_ccmControlGate(info[minor].dev, clk_state_run_wait);

	/* Skip controller initialization if it has been already done by hal */
	if (!(*(uart->base + ctrlr) & (1 << 19 | 1 << 18))) {
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
		t = *(uart->base + baudr) & ~((0x1f << 24) | (1 << 17) | 0x1fff);
		*(uart->base + baudr) = t | calculate_baudrate(UART_BAUDRATE);

		/* Set 8 bit and no parity mode */
		*(uart->base + ctrlr) &= ~0x117;

		/* One stop bit */
		*(uart->base + baudr) &= ~(1 << 13);

		*(uart->base + waterr) = 0;

		/* Enable FIFO */
		*(uart->base + fifor) |= (1 << 7) | (1 << 3);
		*(uart->base + fifor) |= 0x3 << 14;
	}

	/* Clear all status flags */
	*(uart->base + statr) |= 0xc01fc000;

	uart->rxFifoSz = fifoSzLut[*(uart->base + fifor) & 0x7];
	uart->txFifoSz = fifoSzLut[(*(uart->base + fifor) >> 4) & 0x7];

	/* Enable overrun, noise, framing error and receiver interrupts */
	*(uart->base + ctrlr) |= (1 << 27) | (1 << 26) | (1 << 25) | (1 << 21);

	/* Enable TX and RX */
	*(uart->base + ctrlr) |= (1 << 19) | (1 << 18);

	_imxrt_setDevClock(info[minor].dev, clk_state_run);
	hal_interruptsSet(info[minor].irq, uart_handleIntr, (void *)uart);

	lib_printf("\ndev/uart: Initializing uart(%d.%d)", DEV_UART, minor);

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
		.map = uart_map
	};

	devs_register(DEV_UART, UART_MAX_CNT, &h);
}
