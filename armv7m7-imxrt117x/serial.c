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

#include "peripherals.h"
#include "imxrt.h"
#include "../serial.h"
#include "../errors.h"
#include "../timer.h"
#include "../low.h"


#define CONCATENATE(x, y) x##y
#define PIN2MUX(x) CONCATENATE(pctl_mux_gpio_, x)
#define PIN2PAD(x) CONCATENATE(pctl_pad_gpio_, x)

#define UART_MAX_CNT 12

#define UART1_POS 0
#define UART2_POS (UART1_POS + UART1)
#define UART3_POS (UART2_POS + UART2)
#define UART4_POS (UART3_POS + UART3)
#define UART5_POS (UART4_POS + UART4)
#define UART6_POS (UART5_POS + UART5)
#define UART7_POS (UART6_POS + UART6)
#define UART8_POS (UART7_POS + UART7)
#define UART9_POS (UART8_POS + UART8)
#define UART10_POS (UART9_POS + UART9)
#define UART11_POS (UART10_POS + UART10)
#define UART12_POS (UART11_POS + UART11)

#define UART_CNT (UART1 + UART2 + UART3 + UART4 + UART5 + UART6 + \
	UART7 + UART8 + UART9 + UART10 + UART11 + UART12)


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
} serial_t;


struct {
	serial_t serials[UART_CNT];
} serial_common;


const int serialConfig[] = { UART1, UART2, UART3, UART4, UART5, UART6,
	UART7, UART8, UART9, UART10, UART11, UART12 };


const int serialPos[] = { UART1_POS, UART2_POS, UART3_POS, UART4_POS, UART5_POS, UART6_POS,
	UART7_POS, UART8_POS, UART9_POS, UART10_POS, UART11_POS, UART12_POS };


enum { veridr = 0, paramr, globalr, pincfgr, baudr, statr, ctrlr, datar, matchr, modirr, fifor, waterr };


int serial_getRXcount(serial_t *serial)
{
	return (*(serial->base + waterr) >> 24) & 0xff;
}


int serial_getTXcount(serial_t *serial)
{
	return (*(serial->base + waterr) >> 8) & 0xff;
}


int serial_rxEmpty(unsigned int pn)
{
	serial_t *serial;
	--pn;

	if (pn > UART_MAX_CNT || !serialConfig[pn])
		return ERR_ARG;

	serial = &serial_common.serials[serialPos[pn]];

	return serial->rxHead == serial->rxTail;
}


int serial_handleIntr(u16 irq, void *buff)
{
	serial_t *serial = (serial_t *)buff;

	if (serial == NULL)
		return 0;

	/* Receive */
	while (serial_getRXcount(serial)) {
		serial->rxBuff[serial->rxHead] = *(serial->base + datar);
		serial->rxHead = (serial->rxHead + 1) % BUFFER_SIZE;
		if (serial->rxHead == serial->rxTail) {
			serial->rxTail = (serial->rxTail + 1) % BUFFER_SIZE;
		}
	}

	/* Transmit */
	while (serial_getTXcount(serial) < serial->txFifoSz)
	{
		if ((serial->txHead + 1) % BUFFER_SIZE != serial->txTail) {
			serial->txHead = (serial->txHead + 1) % BUFFER_SIZE;
			*(serial->base + datar) = serial->txBuff[serial->txHead];
			serial->tFull = 0;
		}
		else {
			*(serial->base + ctrlr) &= ~(1 << 23);
			break;
		}
	}

	return 0;
}


int serial_read(unsigned int pn, u8 *buff, u16 len, u16 timeout)
{
	serial_t *serial;
	u16 l, cnt;

	--pn;

	if (pn >= (sizeof(serialConfig) / sizeof(serialConfig[0])) || !serialConfig[pn])
		return ERR_ARG;

	serial = &serial_common.serials[serialPos[pn]];

	if (!timer_wait(timeout, TIMER_VALCHG, &serial->rxHead, serial->rxTail))
		return ERR_SERIAL_TIMEOUT;

	low_cli();

	if (serial->rxHead > serial->rxTail)
		l = min(serial->rxHead - serial->rxTail, len);
	else
		l = min(BUFFER_SIZE - serial->rxTail, len);

	low_memcpy(buff, &serial->rxBuff[serial->rxTail], l);
	cnt = l;
	if ((len > l) && (serial->rxHead < serial->rxTail)) {
		low_memcpy(buff + l, &serial->rxBuff[0], min(len - l, serial->rxHead));
		cnt += min(len - l, serial->rxHead);
	}
	serial->rxTail = ((serial->rxTail + cnt) % BUFFER_SIZE);

	low_sti();

	return cnt;
}


int serial_write(unsigned int pn, const u8 *buff, u16 len)
{
	serial_t *serial;
	u16 l, cnt = 0;

	--pn;

	if (pn >= (sizeof(serialConfig) / sizeof(serialConfig[0])) || !serialConfig[pn])
		return ERR_ARG;

	serial = &serial_common.serials[serialPos[pn]];

	while (serial->txHead == serial->txTail && serial->tFull)
		;

	low_cli();
	if (serial->txHead > serial->txTail)
		l = min(serial->txHead - serial->txTail, len);
	else
		l = min(BUFFER_SIZE - serial->txTail, len);

	low_memcpy(&serial->txBuff[serial->txTail], buff, l);
	cnt = l;
	if ((len > l) && (serial->txTail >= serial->txHead)) {
		low_memcpy(serial->txBuff, buff + l, min(len - l, serial->txHead));
		cnt += min(len - l, serial->txHead);
	}

	/* Initialize sending */
	if (serial->txTail == serial->txHead)
		*(serial->base + datar) = serial->txBuff[serial->txHead];

	serial->txTail = ((serial->txTail + cnt) % BUFFER_SIZE);

	if (serial->txTail == serial->txHead)
		serial->tFull = 1;

	*(serial->base + ctrlr) |= 1 << 23;

	low_sti();

	return cnt;
}


int serial_safewrite(unsigned int pn, const u8 *buff, u16 len)
{
	int l;

	for (l = 0; len;) {
		if ((l = serial_write(pn, buff, len)) < 0)
			return ERR_MSG_IO;
		buff += l;
		len -= l;
	}
	return 0;
}



static int serial_muxVal(int uart, int mux)
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


static int serial_getIsel(int uart, int mux, int *isel, int *val)
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


static void serial_initPins(void)
{
	int i, j, isel, val;
	static const struct {
		int uart;
		int muxes[4];
		int pads[4];
	} info[] = {
#if UART1
		{ 0,
		{ PIN2MUX(UART1_TX_PIN), PIN2MUX(UART1_RX_PIN), PIN2MUX(UART1_RTS_PIN), PIN2MUX(UART1_CTS_PIN) },
		{ PIN2PAD(UART1_TX_PIN), PIN2PAD(UART1_RX_PIN), PIN2PAD(UART1_RTS_PIN), PIN2PAD(UART1_CTS_PIN) } },
#endif
#if UART2
		{ 1,
		{ PIN2MUX(UART2_TX_PIN), PIN2MUX(UART2_RX_PIN), PIN2MUX(UART2_RTS_PIN), PIN2MUX(UART2_CTS_PIN) },
		{ PIN2PAD(UART2_TX_PIN), PIN2PAD(UART2_RX_PIN), PIN2PAD(UART2_RTS_PIN), PIN2PAD(UART2_CTS_PIN) } },
#endif
#if UART3
		{ 2,
		{ PIN2MUX(UART3_TX_PIN), PIN2MUX(UART3_RX_PIN), PIN2MUX(UART3_RTS_PIN), PIN2MUX(UART3_CTS_PIN) },
		{ PIN2PAD(UART3_TX_PIN), PIN2PAD(UART3_RX_PIN), PIN2PAD(UART3_RTS_PIN), PIN2PAD(UART3_CTS_PIN) } }, },
#endif
#if UART4
		{ 3,
		{ PIN2MUX(UART4_TX_PIN), PIN2MUX(UART4_RX_PIN), PIN2MUX(UART4_RTS_PIN), PIN2MUX(UART4_CTS_PIN) },
		{ PIN2PAD(UART4_TX_PIN), PIN2PAD(UART4_RX_PIN), PIN2PAD(UART4_RTS_PIN), PIN2PAD(UART4_CTS_PIN) } },
#endif
#if UART5
		{ 4,
		{ PIN2MUX(UART5_TX_PIN), PIN2MUX(UART5_RX_PIN), PIN2MUX(UART5_RTS_PIN), PIN2MUX(UART5_CTS_PIN) },
		{ PIN2PAD(UART5_TX_PIN), PIN2PAD(UART5_RX_PIN), PIN2PAD(UART5_RTS_PIN), PIN2PAD(UART5_CTS_PIN) } },
#endif
#if UART6
		{ 5,
		{ PIN2MUX(UART6_TX_PIN), PIN2MUX(UART6_RX_PIN), PIN2MUX(UART6_RTS_PIN), PIN2MUX(UART6_CTS_PIN) },
		{ PIN2PAD(UART6_TX_PIN), PIN2PAD(UART6_RX_PIN), PIN2PAD(UART6_RTS_PIN), PIN2PAD(UART6_CTS_PIN) } },
#endif
#if UART7
		{ 6,
		{ PIN2MUX(UART7_TX_PIN), PIN2MUX(UART7_RX_PIN), PIN2MUX(UART7_RTS_PIN), PIN2MUX(UART7_CTS_PIN) },
		{ PIN2PAD(UART7_TX_PIN), PIN2PAD(UART7_RX_PIN), PIN2PAD(UART7_RTS_PIN), PIN2PAD(UART7_CTS_PIN) } },
#endif
#if UART8
		{ 7,
		{ PIN2MUX(UART8_TX_PIN), PIN2MUX(UART8_RX_PIN), PIN2MUX(UART8_RTS_PIN), PIN2MUX(UART8_CTS_PIN) },
		{ PIN2PAD(UART8_TX_PIN), PIN2PAD(UART8_RX_PIN), PIN2PAD(UART8_RTS_PIN), PIN2PAD(UART8_CTS_PIN) } },
#endif
#if UART9
		{ 8,
		{ PIN2MUX(UART9_TX_PIN), PIN2MUX(UART9_RX_PIN), PIN2MUX(UART9_RTS_PIN), PIN2MUX(UART9_CTS_PIN) },
		{ PIN2PAD(UART9_TX_PIN), PIN2PAD(UART9_RX_PIN), PIN2PAD(UART9_RTS_PIN), PIN2PAD(UART9_CTS_PIN) } },
#endif
#if UART10
		{ 9,
		{ PIN2MUX(UART10_TX_PIN), PIN2MUX(UART10_RX_PIN), PIN2MUX(UART10_RTS_PIN), PIN2MUX(UART10_CTS_PIN) },
		{ PIN2PAD(UART10_TX_PIN), PIN2PAD(UART10_RX_PIN), PIN2PAD(UART10_RTS_PIN), PIN2PAD(UART10_CTS_PIN) } },
#endif
#if UART11
		{ 10,
		{ PIN2MUX(UART11_TX_PIN), PIN2MUX(UART11_RX_PIN), PIN2MUX(UART11_RTS_PIN), PIN2MUX(UART11_CTS_PIN) },
		{ PIN2PAD(UART11_TX_PIN), PIN2PAD(UART11_RX_PIN), PIN2PAD(UART11_RTS_PIN), PIN2PAD(UART11_CTS_PIN) } },
#endif
#if UART12
		{ 11,
		{ PIN2MUX(UART12_TX_PIN), PIN2MUX(UART12_RX_PIN), PIN2MUX(UART12_RTS_PIN), PIN2MUX(UART12_CTS_PIN) },
		{ PIN2PAD(UART12_TX_PIN), PIN2PAD(UART12_RX_PIN), PIN2PAD(UART12_RTS_PIN), PIN2PAD(UART12_CTS_PIN) } },
#endif
	};

	for (i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
		for (j = 0; j < sizeof(info[0].muxes) / sizeof(info[0].muxes[0]); ++j) {
			_imxrt_setIOmux(info[i].muxes[j], 0, serial_muxVal(info[i].uart, info[i].muxes[j]));

			if (serial_getIsel(info[i].uart, info[i].muxes[j], &isel, &val) >= 0)
				_imxrt_setIOisel(isel, val);
		}

		_imxrt_setIOpad(info[i].pads[0], 0, 0, 0, 0, 0, 0);
		_imxrt_setIOpad(info[i].pads[1], 0, 0, 1, 1, 0, 0);
		_imxrt_setIOpad(info[i].pads[2], 0, 0, 0, 0, 0, 0);
		_imxrt_setIOpad(info[i].pads[3], 0, 0, 1, 1, 0, 0);
	}
}


/* TODO: set baudrate using 'baud' argument */
void serial_init(u32 baud, u32 *st)
{
	u32 t;
	int i, dev;
	serial_t *serial;

	static const u32 fifoSzLut[] = { 1, 4, 8, 16, 32, 64, 128, 256 };
	static const struct {
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
		{ UART8_BASE, UART8_CLK, UART8_IRQ },
		{ UART9_BASE, UART9_CLK, UART9_IRQ },
		{ UART10_BASE, UART10_CLK, UART10_IRQ },
		{ UART11_BASE, UART11_CLK, UART11_IRQ },
		{ UART12_BASE, UART12_CLK, UART12_IRQ }
	};

	*st = 115200;
	serial_initPins();

	for (i = 0, dev = 0; dev < sizeof(serialConfig) / sizeof(serialConfig[0]); ++dev) {
		if (!serialConfig[dev])
			continue;

		_imxrt_setDevClock(info[dev].dev, 0, 0, 0, 0, 1);

		serial = &serial_common.serials[i++];
		serial->base = info[dev].base;
		serial->rxHead = 0;
		serial->txHead = 0;
		serial->rxTail = 0;

		serial->txTail = 0;
		serial->tFull = 0;

		/* Disable TX and RX */
		*(serial->base + ctrlr) &= ~((1 << 19) | (1 << 18));

		/* Reset all internal logic and registers, except the Global Register */
		*(serial->base + globalr) |= 1 << 1;
		imxrt_dataBarrier();
		*(serial->base + globalr) &= ~(1 << 1);
		imxrt_dataBarrier();

		/* Disable input trigger */
		*(serial->base + pincfgr) &= ~3;

		/* Set 115200 default baudrate */
		t = *(serial->base + baudr) & ~((0x1f << 24) | (1 << 17) | 0xfff);
		*(serial->base + baudr) = t | calculate_baudrate(115200);

		/* Set 8 bit and no parity mode */
		*(serial->base + ctrlr) &= ~0x117;

		/* One stop bit */
		*(serial->base + baudr) &= ~(1 << 13);

		*(serial->base + waterr) = 0;

		/* Enable FIFO */
		*(serial->base + fifor) |= (1 << 7) | (1 << 3);
		*(serial->base + fifor) |= 0x3 << 14;

		/* Clear all status flags */
		*(serial->base + statr) |= 0xc01fc000;

		serial->rxFifoSz = fifoSzLut[*(serial->base + fifor) & 0x7];
		serial->txFifoSz = fifoSzLut[(*(serial->base + fifor) >> 4) & 0x7];

		/* Enable receiver interrupt */
		*(serial->base + ctrlr) |= 1 << 21;

		/* Enable TX and RX */
		*(serial->base + ctrlr) |= (1 << 19) | (1 << 18);

		low_irqinst(info[dev].irq, serial_handleIntr, (void *)serial);
	}

	return;
}


void serial_done(void)
{
	int i, dev;
	serial_t *serial;

	for (i = 0, dev = 0; dev < sizeof(serialConfig) / sizeof(serialConfig[0]); ++dev) {
		if (!serialConfig[dev])
			continue;

		serial = &serial_common.serials[i++];

		/* disable TX and RX */
		*(serial->base + ctrlr) &= ~((1 << 19) | (1 << 18));
		*(serial->base + ctrlr) &= ~((1 << 23) | (1 << 21));

		low_irquninst(serial->irq);
	}
}
