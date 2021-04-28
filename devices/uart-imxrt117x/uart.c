/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MXRT1176 Serial driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczyński, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../../errors.h"
#include "../../timer.h"
#include "../../hal.h"

#include "peripherals.h"
#include "imxrt.h"
#include "uart.h"
#include "devs.h"

#define CONCATENATE(x, y) x##y
#define PIN2MUX(x) CONCATENATE(pctl_mux_gpio_, x)
#define PIN2PAD(x) CONCATENATE(pctl_pad_gpio_, x)

#define UART_MAX_CNT 12

#define BUFFER_SIZE 0x200


typedef struct {
	volatile u32 *base;
	u16 irq;

	u16 rxFifoSz;
	u16 txFifoSz;

	u8 rxBuff[BUFFER_SIZE];
	u16 rxHead;
	u16 rxTail;

	u8 txBuff[BUFFER_SIZE];
	u16 txHead;
	u16 txTail;
	u8 tFull;
} uart_t;


struct {
	uart_t uarts[UART_MAX_CNT];
	int init;
} uart_common;


enum { veridr = 0, paramr, globalr, pincfgr, baudr, statr, ctrlr, datar, matchr, modirr, fifor, waterr };


static const u32 fifoSzLut[] = { 1, 4, 8, 16, 32, 64, 128, 256 };

const int uartLut[] = { UART1, UART2, UART3, UART4, UART5, UART6, UART7, UART8, UART9, UART10, UART11, UART12 };

static const struct {
	volatile u32 *base;
	int dev;
	unsigned irq;
} infoUarts[] = {
	{ UART1_BASE, UART1_CLK, UART1_IRQ },
	{ UART2_BASE, UART2_CLK, UART2_IRQ },
	{ UART3_BASE, UART3_CLK, UART3_IRQ },
	{ UART4_BASE, UART4_CLK, UART4_IRQ },
	{ UART5_BASE, UART5_CLK, UART5_IRQ },
	{ UART6_BASE, UART6_CLK, UART6_IRQ },
	{ UART7_BASE, UART7_CLK, UART7_IRQ },
	{ UART8_BASE, UART8_CLK, UART8_IRQ },
	{ UART9_BASE, UART9_CLK, UART9_IRQ },
	{ UART10_BASE, UART10_CLK, UART10_IRQ },
	{ UART11_BASE, UART11_CLK, UART11_IRQ },
	{ UART12_BASE, UART12_CLK, UART12_IRQ }
};


/* TODO: temporary solution, it should be moved to device tree */
static uart_t *uart_getInstance(unsigned int dn)
{
	if (dn < 1 || dn > UART_MAX_CNT)
 		return NULL;

	if (uartLut[dn - 1] == 0)
		return NULL;

	return &uart_common.uarts[dn - 1];
}


int uart_getRXcount(uart_t *uart)
{
	return (*(uart->base + waterr) >> 24) & 0xff;
}


int uart_getTXcount(uart_t *uart)
{
	return (*(uart->base + waterr) >> 8) & 0xff;
}


int uart_rxEmpty(unsigned int dn)
{
	uart_t *uart;

	if ((uart = uart_getInstance(dn)) == NULL)
		return ERR_ARG;

	return uart->rxHead == uart->rxTail;
}


int uart_handleIntr(u16 irq, void *buff)
{
	uart_t *uart = (uart_t *)buff;

	if (uart == NULL)
		return 0;

	/* Receive */
	while (uart_getRXcount(uart)) {
		uart->rxBuff[uart->rxHead] = *(uart->base + datar);
		uart->rxHead = (uart->rxHead + 1) % BUFFER_SIZE;
		if (uart->rxHead == uart->rxTail) {
			uart->rxTail = (uart->rxTail + 1) % BUFFER_SIZE;
		}
	}

	/* Transmit */
	while (uart_getTXcount(uart) < uart->txFifoSz)
	{
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


/* TODO: when hal_console will be introduced, uart_read should be replaced by dev interface */
int uart_read(unsigned int dn, u8 *buff, u16 len, u16 timeout)
{
	uart_t *uart;
	u16 l, cnt;

	if ((uart = uart_getInstance(dn)) == NULL)
		return ERR_ARG;

	if (!timer_wait(timeout, TIMER_VALCHG, &uart->rxHead, uart->rxTail))
		return ERR_UART_TIMEOUT;

	hal_cli();

	if (uart->rxHead > uart->rxTail)
		l = min(uart->rxHead - uart->rxTail, len);
	else
		l = min(BUFFER_SIZE - uart->rxTail, len);

	hal_memcpy(buff, &uart->rxBuff[uart->rxTail], l);
	cnt = l;
	if ((len > l) && (uart->rxHead < uart->rxTail)) {
		hal_memcpy(buff + l, &uart->rxBuff[0], min(len - l, uart->rxHead));
		cnt += min(len - l, uart->rxHead);
	}
	uart->rxTail = ((uart->rxTail + cnt) % BUFFER_SIZE);

	hal_sti();

	return cnt;
}


/* TODO: when hal_console will be introduced, uart_write should be replaced by dev interface */
int uart_write(unsigned int dn, const u8 *buff, u16 len)
{
	uart_t *uart;
	u16 l, cnt = 0;

	if ((uart = uart_getInstance(dn)) == NULL)
		return ERR_ARG;

	while (uart->txHead == uart->txTail && uart->tFull)
		;

	hal_cli();
	if (uart->txHead > uart->txTail)
		l = min(uart->txHead - uart->txTail, len);
	else
		l = min(BUFFER_SIZE - uart->txTail, len);

	hal_memcpy(&uart->txBuff[uart->txTail], buff, l);
	cnt = l;
	if ((len > l) && (uart->txTail >= uart->txHead)) {
		hal_memcpy(uart->txBuff, buff + l, min(len - l, uart->txHead));
		cnt += min(len - l, uart->txHead);
	}

	/* Initialize sending */
	if (uart->txTail == uart->txHead)
		*(uart->base + datar) = uart->txBuff[uart->txHead];

	uart->txTail = ((uart->txTail + cnt) % BUFFER_SIZE);

	if (uart->txTail == uart->txHead)
		uart->tFull = 1;

	*(uart->base + ctrlr) |= 1 << 23;

	hal_sti();

	return cnt;
}


int uart_safewrite(unsigned int pn, const u8 *buff, u16 len)
{
	int l;

	for (l = 0; len;) {
		if ((l = uart_write(pn, buff, len)) < 0)
			return ERR_MSG_IO;
		buff += l;
		len -= l;
	}
	return 0;
}


static int uart_muxVal(int uart, int mux)
{
	if (mux == pctl_mux_gpio_ad_02)
		return (uart == 6) ? 1 : 6;

	if (mux == pctl_mux_gpio_ad_03)
		return (uart == 6) ? 1 : 6;

	if (mux == pctl_mux_gpio_disp_b2_08)
		return (uart == 0) ? 9 : 2;

	if (mux == pctl_mux_gpio_disp_b2_09)
		return (uart == 0) ? 9 : 2;

	switch (mux) {
		case pctl_mux_gpio_ad_24:
		case pctl_mux_gpio_ad_25:
		case pctl_mux_gpio_ad_26:
		case pctl_mux_gpio_ad_27:
		case pctl_mux_gpio_lpsr_08:
		case pctl_mux_gpio_lpsr_09:
			return 0;

		case pctl_mux_gpio_ad_04:
		case pctl_mux_gpio_ad_05:
		case pctl_mux_gpio_ad_15:
		case pctl_mux_gpio_ad_16:
		case pctl_mux_gpio_ad_28:
		case pctl_mux_gpio_ad_29:
			return 1;

		case pctl_mux_gpio_disp_b1_04:
		case pctl_mux_gpio_disp_b1_05:
		case pctl_mux_gpio_disp_b1_06:
		case pctl_mux_gpio_disp_b1_07:
		case pctl_mux_gpio_disp_b2_06:
		case pctl_mux_gpio_disp_b2_07:
		case pctl_mux_gpio_disp_b2_10:
		case pctl_mux_gpio_disp_b2_11:
			return 2;

		case pctl_mux_gpio_disp_b2_12:
		case pctl_mux_gpio_disp_b2_13:
		case pctl_mux_gpio_sd_b2_00:
		case pctl_mux_gpio_sd_b2_01:
		case pctl_mux_gpio_sd_b2_02:
		case pctl_mux_gpio_sd_b2_03:
		case pctl_mux_gpio_sd_b2_07:
		case pctl_mux_gpio_sd_b2_08:
		case pctl_mux_gpio_sd_b2_09:
		case pctl_mux_gpio_sd_b2_10:
		case pctl_mux_gpio_emc_b1_40:
		case pctl_mux_gpio_emc_b1_41:
		case pctl_mux_gpio_emc_b2_00:
		case pctl_mux_gpio_emc_b2_01:
		case pctl_mux_gpio_lpsr_06:
		case pctl_mux_gpio_lpsr_07:
			return 3;

		case pctl_mux_gpio_ad_30:
		case pctl_mux_gpio_ad_31:
			return 4;

		case pctl_mux_gpio_ad_00:
		case pctl_mux_gpio_ad_01:
		case pctl_mux_gpio_lpsr_00:
		case pctl_mux_gpio_lpsr_01:
		case pctl_mux_gpio_lpsr_04:
		case pctl_mux_gpio_lpsr_05:
			return 6;

		case pctl_mux_gpio_ad_32:
		case pctl_mux_gpio_ad_33:
		case pctl_mux_gpio_ad_34:
		case pctl_mux_gpio_ad_35:
		case pctl_mux_gpio_lpsr_10:
		case pctl_mux_gpio_lpsr_11:
			return 8;

		case pctl_mux_gpio_disp_b1_02:
		case pctl_mux_gpio_disp_b1_03:
			return 9;
	}

	return 2;
}


static u32 calculate_baudrate(int baud)
{
	int osr, sbr, bestSbr = 0, bestOsr = 0, bestErr = 1000, t;

	if (!baud)
		return 0;

	for (osr = 3; osr < 32; ++osr) {
		sbr = UART_CLK / (baud * (osr + 1));
		sbr &= 0xfff;
		t = UART_CLK / (sbr * (osr + 1));

		if (t > baud)
			t = ((t - baud) * 1000) / baud;
		else
			t = ((baud - t) * 1000) / baud;

		if (t < bestErr) {
			bestErr = t;
			bestOsr = osr;
			bestSbr = sbr;
		}

		/* Finish if error is < 1% */
		if (bestErr < 10)
			break;
	}

	return (bestOsr << 24) | ((bestOsr <= 6) << 17) | bestSbr;
}


static int uart_getIsel(int uart, int mux, int *isel, int *val)
{
	switch (mux) {
		case pctl_mux_gpio_ad_24:      *isel = pctl_isel_lpuart1_txd; *val = 0; break;
		case pctl_mux_gpio_disp_b1_02: *isel = pctl_isel_lpuart1_txd; *val = 1; break;
		case pctl_mux_gpio_ad_25:      *isel = pctl_isel_lpuart1_rxd; *val = 0; break;
		case pctl_mux_gpio_disp_b1_03: *isel = pctl_isel_lpuart1_rxd; *val = 1; break;
		case pctl_mux_gpio_disp_b2_06: *isel = pctl_isel_lpuart7_txd; *val = 1; break;
		case pctl_mux_gpio_ad_00:      *isel = pctl_isel_lpuart7_txd; *val = 0; break;
		case pctl_mux_gpio_disp_b2_07: *isel = pctl_isel_lpuart7_rxd; *val = 1; break;
		case pctl_mux_gpio_ad_01:      *isel = pctl_isel_lpuart7_rxd; *val = 0; break;
		case pctl_mux_gpio_ad_02:      *isel = pctl_isel_lpuart8_txd; *val = 0; break;
		case pctl_mux_gpio_ad_03:      *isel = pctl_isel_lpuart8_rxd; *val = 0; break;
		case pctl_mux_gpio_lpsr_08:    *isel = pctl_isel_lpuart11_txd; *val = 1; break;
		case pctl_mux_gpio_lpsr_04:    *isel = pctl_isel_lpuart11_txd; *val = 0; break;
		case pctl_mux_gpio_lpsr_09:    *isel = pctl_isel_lpuart11_rxd; *val = 1; break;
		case pctl_mux_gpio_lpsr_05:    *isel = pctl_isel_lpuart11_rxd; *val = 0; break;
		case pctl_mux_gpio_lpsr_06:    *isel = pctl_isel_lpuart12_txd; *val = 1; break;
		case pctl_mux_gpio_lpsr_00:    *isel = pctl_isel_lpuart12_txd; *val = 0; break;
		case pctl_mux_gpio_lpsr_10:    *isel = pctl_isel_lpuart12_txd; *val = 2; break;
		case pctl_mux_gpio_lpsr_07:    *isel = pctl_isel_lpuart12_rxd; *val = 1; break;
		case pctl_mux_gpio_lpsr_01:    *isel = pctl_isel_lpuart12_rxd; *val = 0; break;
		case pctl_mux_gpio_lpsr_11:    *isel = pctl_isel_lpuart12_rxd; *val = 2; break;
		case pctl_mux_gpio_disp_b2_09:
			if (uart == 0) {
				*isel = pctl_isel_lpuart1_rxd;
				*val = 2;
			}
			else {
				*isel = pctl_isel_lpuart8_rxd;
				*val = 1;
			}
			break;
		case pctl_mux_gpio_disp_b2_08:
			if (uart == 0) {
				*isel = pctl_isel_lpuart1_txd;
				*val = 2;
			}
			else {
				*isel = pctl_isel_lpuart8_txd;
				*val = 1;
			}
			break;
		default: return -1;
	}

	return 0;
}


static void uart_initPins(void)
{
	int i, j, isel, val;
	static const struct {
		int uart;
		int muxes[2];
		int pads[2];
	} info[] = {
#if UART1
		{ 0,
		{ PIN2MUX(UART1_TX_PIN), PIN2MUX(UART1_RX_PIN) },
		{ PIN2PAD(UART1_TX_PIN), PIN2PAD(UART1_RX_PIN) } },
#endif
#if UART2
		{ 1,
		{ PIN2MUX(UART2_TX_PIN), PIN2MUX(UART2_RX_PIN) },
		{ PIN2PAD(UART2_TX_PIN), PIN2PAD(UART2_RX_PIN) } },
#endif
#if UART3
		{ 2,
		{ PIN2MUX(UART3_TX_PIN), PIN2MUX(UART3_RX_PIN) },
		{ PIN2PAD(UART3_TX_PIN), PIN2PAD(UART3_RX_PIN) } },
#endif
#if UART4
		{ 3,
		{ PIN2MUX(UART4_TX_PIN), PIN2MUX(UART4_RX_PIN) },
		{ PIN2PAD(UART4_TX_PIN), PIN2PAD(UART4_RX_PIN) } },
#endif
#if UART5
		{ 4,
		{ PIN2MUX(UART5_TX_PIN), PIN2MUX(UART5_RX_PIN) },
		{ PIN2PAD(UART5_TX_PIN), PIN2PAD(UART5_RX_PIN) } },
#endif
#if UART6
		{ 5,
		{ PIN2MUX(UART6_TX_PIN), PIN2MUX(UART6_RX_PIN) },
		{ PIN2PAD(UART6_TX_PIN), PIN2PAD(UART6_RX_PIN) } },
#endif
#if UART7
		{ 6,
		{ PIN2MUX(UART7_TX_PIN), PIN2MUX(UART7_RX_PIN) },
		{ PIN2PAD(UART7_TX_PIN), PIN2PAD(UART7_RX_PIN) } },
#endif
#if UART8
		{ 7,
		{ PIN2MUX(UART8_TX_PIN), PIN2MUX(UART8_RX_PIN) },
		{ PIN2PAD(UART8_TX_PIN), PIN2PAD(UART8_RX_PIN) } },
#endif
#if UART9
		{ 8,
		{ PIN2MUX(UART9_TX_PIN), PIN2MUX(UART9_RX_PIN) },
		{ PIN2PAD(UART9_TX_PIN), PIN2PAD(UART9_RX_PIN) } },
#endif
#if UART10
		{ 9,
		{ PIN2MUX(UART10_TX_PIN), PIN2MUX(UART10_RX_PIN) },
		{ PIN2PAD(UART10_TX_PIN), PIN2PAD(UART10_RX_PIN) } },
#endif
#if UART11
		{ 10,
		{ PIN2MUX(UART11_TX_PIN), PIN2MUX(UART11_RX_PIN) },
		{ PIN2PAD(UART11_TX_PIN), PIN2PAD(UART11_RX_PIN) } },
#endif
#if UART12
		{ 11,
		{ PIN2MUX(UART12_TX_PIN), PIN2MUX(UART12_RX_PIN) },
		{ PIN2PAD(UART12_TX_PIN), PIN2PAD(UART12_RX_PIN) } },
#endif
	};

	for (i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
		for (j = 0; j < sizeof(info[0].muxes) / sizeof(info[0].muxes[0]); ++j) {
			_imxrt_setIOmux(info[i].muxes[j], 0, uart_muxVal(info[i].uart, info[i].muxes[j]));

			if (uart_getIsel(info[i].uart, info[i].muxes[j], &isel, &val) >= 0)
				_imxrt_setIOisel(isel, val);
		}

		_imxrt_setIOpad(info[i].pads[0], 0, 0, 0, 0, 0, 0);
		_imxrt_setIOpad(info[i].pads[1], 0, 0, 1, 1, 0, 0);
	}
}


u32 uart_getBaudrate(void)
{
	return UART_BAUDRATE;
}


/* Device interafce */

ssize_t uart_devRead(unsigned int dn, addr_t offs, u8 *buff, unsigned int len)
{
	uart_t *uart;

	if ((uart = uart_getInstance(dn)) == NULL)
		return ERR_ARG;

	return uart_read(dn, buff, len, 500);
}


ssize_t uart_devWrite(unsigned int dn, addr_t offs, const u8 *buff, unsigned int len)
{
	uart_t *uart;

	if ((uart = uart_getInstance(dn)) == NULL)
		return ERR_ARG;

	return uart_write(dn, buff, len);
}


int uart_sync(unsigned int dn)
{
	uart_t *uart;

	if ((uart = uart_getInstance(dn)) == NULL)
		return ERR_ARG;

	/* TBD */

	return ERR_NONE;
}


int uart_deinit(unsigned int dn)
{
	uart_t *uart;

	if ((uart = uart_getInstance(dn)) == NULL)
		return ERR_ARG;

	/* disable TX and RX */
	*(uart->base + ctrlr) &= ~((1 << 19) | (1 << 18));
	*(uart->base + ctrlr) &= ~((1 << 23) | (1 << 21));

	hal_irquninst(uart->irq);

	return ERR_NONE;
}


int uart_init(unsigned int dn, dev_handler_t *h)
{
	u32 t, id;
	uart_t *uart;

	if ((uart = uart_getInstance(dn)) == NULL)
		return ERR_ARG;

	if (uart_common.init == 0) {
		uart_initPins();
		uart_common.init = 1;
	}

	id = dn - 1;
	uart->base = infoUarts[id].base;

	_imxrt_setDevClock(infoUarts[id].dev, 0, 0, 0, 0, 1);

	/* Disable TX and RX */
	*(uart->base + ctrlr) &= ~((1 << 19) | (1 << 18));

	/* Reset all internal logic and registers, except the Global Register */
	*(uart->base + globalr) |= 1 << 1;
	imxrt_dataBarrier();
	*(uart->base + globalr) &= ~(1 << 1);
	imxrt_dataBarrier();

	/* Disable input trigger */
	*(uart->base + pincfgr) &= ~3;

	/* Set 115200 default baudrate */
	t = *(uart->base + baudr) & ~((0x1f << 24) | (1 << 17) | 0xfff);
	*(uart->base + baudr) = t | calculate_baudrate(UART_BAUDRATE);

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

	hal_irqinst(infoUarts[id].irq, uart_handleIntr, (void *)uart);

	h->deinit = uart_deinit;
	h->sync = uart_sync;
	h->read = uart_devRead;
	h->write = uart_devWrite;

	return ERR_NONE;
}


__attribute__((constructor)) static void uart_reg(void)
{
	devs_regDriver(DEV_UART, uart_init);
}
