/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Quad-SPI Controller driver
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "qspi.h"
#include <lib/errno.h>


enum { cr = 0, sr, ier, idr, imr, er, dr, txd00, rxd, sicr, txth, rxth, gpio,
	   lpbk = 0xe, txd01 = 0x20, txd10, txd11,
	   lqspi_cr = 0x28, lqspi_sr, modid = 0x3f };


struct {
	volatile u32 *base;
} qspi_common;


void qspi_stop(void)
{
	/* Clean up RX Fifo */
	while ((*(qspi_common.base + sr) & (1 << 4)))
		*(qspi_common.base + rxd);

	*(qspi_common.base + cr) |= (1 << 10);
	hal_cpuDataMemoryBarrier();

	*(qspi_common.base + er) = 0;
	hal_cpuDataMemoryBarrier();
}


void qspi_start(void)
{
	*(qspi_common.base + rxth) = 0x1;
	*(qspi_common.base + cr) &= ~(1 << 10);
	hal_cpuDataMemoryBarrier();

	*(qspi_common.base + er) = 0x1;
	hal_cpuDataMemoryBarrier();
}


static unsigned int qspi_rxData(u8 *rxBuff, size_t size)
{
	const u32 data = *(qspi_common.base + rxd);

	if (rxBuff != NULL) {
		switch (size) {
			case 0:
				break;

			case 1:
				rxBuff[0] = data >> 24;
				break;

			case 2:
				rxBuff[0] = (data >> 16) & 0xff;
				rxBuff[1] = data >> 24;
				break;

			case 3:
				rxBuff[0] = (data >> 8) & 0xff;
				rxBuff[1] = (data >> 16) & 0xff;
				rxBuff[2] = data >> 24;
				break;

			case 4:
			default:
				rxBuff[0] = data & 0xff;
				rxBuff[1] = (data >> 8) & 0xff;
				rxBuff[2] = (data >> 16) & 0xff;
				rxBuff[3] = data >> 24;
				break;
		}
	}

	return (size < 4) ? size : 4;
}


static unsigned int qspi_txData(const u8 *txBuff, size_t size)
{
	const u8 dummy[sizeof(u32)] = { 0 };
	const u8 *buff = (txBuff == NULL) ? dummy : txBuff;

	switch (size) {
		case 0:
			return 0;

		case 1:
			*(qspi_common.base + txd01) = buff[0];
			return 1;

		case 2:
			*(qspi_common.base + txd10) = buff[0] | (buff[1] << 8);
			return 2;

		case 3:
			*(qspi_common.base + txd11) = buff[0] | (buff[1] << 8) | (buff[2] << 16);
			return 3;

		case 4:
		default:
			*(qspi_common.base + txd00) = buff[0] | (buff[1] << 8) | (buff[2] << 16) | (buff[3] << 24);
			return 4;
	}
}


ssize_t qspi_polledTransfer(const u8 *txBuff, u8 *rxBuff, size_t size, time_t timeout)
{
	time_t start;
	size_t tempSz, txSz = size, rxSz = 0;

	start = hal_timerGet();

	while (txSz || rxSz) {
		/* Transmit data */
		while (txSz) {
			/* Incomplete word has to be send and receive as a last transfer
			 * otherwise there is an undefined behaviour.                   */
			if ((txSz < sizeof(u32)) && (rxSz >= sizeof(u32)))
				break;

			/* TX Fifo is full */
			if ((*(qspi_common.base + sr) & (1 << 3)))
				break;

			tempSz = qspi_txData(txBuff, txSz);

			txSz -= tempSz;
			rxSz += tempSz;

			if (txBuff != NULL)
				txBuff += tempSz;
		}

		/* Start data transmission */
		*(qspi_common.base + cr) |= (1 << 16);

		/* Wait until TX Fifo is empty */
		while ((*(qspi_common.base + sr) & (1 << 2)) == 0) {
			if ((hal_timerGet() - start) >= timeout)
				return -ETIME;
		}

		/* Receive data */
		while (rxSz) {
			/* RX Fifo is empty */
			if ((*(qspi_common.base + sr) & (1 << 4)) == 0)
				break;

			tempSz = qspi_rxData(rxBuff, rxSz);
			rxSz -= tempSz;

			if (rxBuff != NULL)
				rxBuff += tempSz;
		}
	}

	return size - txSz;
}


static int qspi_setPin(u32 pin)
{
	ctl_mio_t ctl;

	ctl.pin = pin;
	ctl.l0 = 0x1;
	ctl.l1 = ctl.l2 = ctl.l3 = 0;
	ctl.pullup = 0;
	ctl.speed = 0x1;
	ctl.ioType = 0x1;
	ctl.disableRcvr = 0;
	ctl.triEnable = 0;

	if (pin == mio_pin_01) {
		ctl.pullup = 1;
		ctl.speed = 0x0;
	}
	else if (pin < mio_pin_01 && pin > mio_pin_08) {
		return -EINVAL;
	}

	return _zynq_setMIO(&ctl);
}


static int qspi_initCtrlClk(void)
{
	ctl_clock_t ctl;

	/* Set IO PLL as source clock and set divider:
	 * IO_PLL / 0x5 :  1000 MHz / 5 = 200 MHz     */
	ctl.dev = ctrl_lqspi_clk;
	ctl.pll.clkact0 = 0x1;
	ctl.pll.clkact1 = 0x1;
	ctl.pll.srcsel = 0;
	ctl.pll.divisor0 = 5;

	return _zynq_setCtlClock(&ctl);
}


/* Linear mode allows only for reading data.
 * 03h command is recommended, otherwise first word = 0 (internal bug) :
 * https://support.xilinx.com/s/article/60803?language=en_US
 */
#if 0
static int qspi_linearMode(void)
{
	/* Disable QSPI */
	*(qspi_common.base + er) &= ~0x1;
	hal_cpuDataMemoryBarrier();

	/* Disable IRQs */
	*(qspi_common.base + idr) = 0x7d;

	/* Disable linear mode */
	*(qspi_common.base + lqspi_cr) = 0;

	*(qspi_common.base + cr) = (1 << 14);
	*(qspi_common.base + cr) = (1 << 10);

	*(qspi_common.base + cr) &= ~(0x7 << 3);
	*(qspi_common.base + cr) |= (0x3 << 3);

	*(qspi_common.base + cr) |= 0x1;
	*(qspi_common.base + cr) |= (1 << 31);
	*(qspi_common.base + cr) &= ~(1 << 26);

	*(qspi_common.base + cr) |= (0x3 << 6);
	*(qspi_common.base + cr) &= ~(0x3 << 1);
	*(qspi_common.base + cr) |= (0x1 << 19);

	*(qspi_common.base + lqspi_cr) =  0x80000003;

	*(qspi_common.base + er) = 0x1;
	hal_cpuDataMemoryBarrier();

	return EOK;
}
#endif


static void qspi_IOMode(void)
{
	/* Configure I/O mode */

	/* Disable QSPI */
	*(qspi_common.base + er) &= ~0x1;
	hal_cpuDataMemoryBarrier();

	/* Disable IRQs */
	*(qspi_common.base + idr) = 0x7d;

	/* Configure controller */

	/* Set baud rate to 100 MHz: 200 MHz / 2, master mode, not Legacy mode */
	*(qspi_common.base + cr) &= ~(0x7 << 3);
	*(qspi_common.base + cr) |= (0x0 << 3) | 0x1 | (1 << 31);
	/* Set little endian */
	*(qspi_common.base + cr) &= ~(1 << 26);
	/* Set FIFO width 32 bits */
	*(qspi_common.base + cr) |= (0x3 << 6);
	/* Set clock phase and polarity */
	*(qspi_common.base + cr) &= ~(0x3 << 1);


	/* Enable manual mode and manual CS */
	*(qspi_common.base + cr) |= (0x3 << 14);

	/* Loopback clock is used for high-speed read data capturing (>40MHz) */
	*(qspi_common.base + lpbk) = *(qspi_common.base + lpbk) & ~0x3f;
	*(qspi_common.base + lpbk) = (1 << 5);

	/* Disable linear mode */
	*(qspi_common.base + lqspi_cr) = 0;
	hal_cpuDataMemoryBarrier();
}


int qspi_deinit(void)
{
	qspi_stop();

	return _zynq_setAmbaClk(amba_lqspi_clk, clk_disable);
}


int qspi_init(void)
{
	int res;

	qspi_common.base = (void *)0xe000d000;

	res = _zynq_setAmbaClk(amba_lqspi_clk, clk_enable);
	if (res < 0)
		return res;

	res = qspi_initCtrlClk();
	if (res < 0)
		return res;

	qspi_setPin(mio_pin_01); /* Chip Select */
	qspi_setPin(mio_pin_02); /* I/O */
	qspi_setPin(mio_pin_03); /* I/O */
	qspi_setPin(mio_pin_04); /* I/O */
	qspi_setPin(mio_pin_05); /* I/O */
	qspi_setPin(mio_pin_06); /* CLK */
	qspi_setPin(mio_pin_08); /* SCLK */

	qspi_IOMode();

	return EOK;
}
