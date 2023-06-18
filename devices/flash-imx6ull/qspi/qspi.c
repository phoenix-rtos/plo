/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * QSPI Controller driver
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <hal/hal.h>
#include <lib/lib.h>

#include "../qspi.h"
#include "../lut.h"
#include "../nor/nor.h"

#define QSPI_MMAP_BASE 0x60000000u

#define QSPI_BASE ((volatile u32 *)0x21e0000)

#define QSPI_LCKCR        (0x304u / sizeof(u32))
#define QSPI_LCKCR_LOCK   0x01u
#define QSPI_LCKCR_UNLOCK 0x02u

#define QSPI_LUTKEY     (0x300u / sizeof(u32))
#define QSPI_LUTKEY_KEY 0x5af05af0u

#define QSPI_LUT (0x310u / sizeof(u32))

#define QSPI_IPCR (0x08u / sizeof(u32))

#define QSPI_FLSHCR (0x0Cu / sizeof(u32))

#define QSPI_MCR          (0u)
#define QSPI_MCR_CLR_TXF  (0x01u << 11)
#define QSPI_MCR_CLR_RXF  (0x01u << 10)
#define QSPI_MCR_CLR_MDIS (0x01u << 14)
#define QSPI_MCR_SWRSTSD  (0x01u)
#define QSPI_MCR_SWRSTHD  (0x01u << 1)

#define QSPI_BFGENCR (0x20u / sizeof(u32))

#define QSPI_SPTRCLR     (0x16cu / sizeof(u32))
#define QSPI_SPTRCLR_IP  (1u << 8)
#define QSPI_SPTRCLR_AHB (1u)

#define QSPI_BUFIND(n) ((0x30u + (4u * (n))) / sizeof(u32))

#define QSPI_BUFCR(n) ((0x10u + (4u * (n))) / sizeof(u32))

#define QSPI_BUFCR_INVALID_MASTER (0xeu)

#define QSPI_SR        (0x15cu / sizeof(u32))
#define QSPI_SR_BUSY   (0x01u)
#define QSPI_SR_RXWE   (0x01u << 16)
#define QSPI_SR_TXEDA  (0x01u << 24)
#define QSPI_SR_TXFULL (0x01u << 27)
#define QSPI_SR_IP_ACC (0x01u << 1)

#define QSPI_RBDRn(n) ((0x200u + (4u * (n))) / sizeof(u32))

#define QSPI_RBSR (0x10cu / sizeof(u32))

#define QSPI_SFAR (0x100u / sizeof(u32))

#define QSPI_RBCT       (0x110u / sizeof(u32))
#define QSPI_RBCT_RXBRD (0x01u << 8)

#define QSPI_FR       (0x160u / sizeof(u32))
#define QSPI_FR_RBDF  (0x01u << 16)
#define QSPI_FR_IPIEF (0x01u << 6)
#define QSPI_FR_IPAEF (0x01u << 7)

#define QSPI_TBDR (0x154u / sizeof(u32))

#define QSPI_SFA1AD (0x180u / sizeof(u32))
#define QSPI_SFA2AD (0x184u / sizeof(u32))
#define QSPI_SFB1AD (0x188u / sizeof(u32))
#define QSPI_SFB2AD (0x18cu / sizeof(u32))

#define WATERMARK 16u


__attribute__((section(".noxip"))) static void qspi_setMux(int dev_no)
{
	size_t i;
	static const struct {
		int mux;
		char mode;
		char sion;
	} qspi_mux[2][8] = {
		{ { mux_nand_d7, 2, 0 }, { mux_nand_ale, 2, 0 }, { mux_nand_wp, 2, 0 }, { mux_nand_rdy, 2, 0 }, { mux_nand_ce0, 2, 0 }, { mux_nand_ce1, 2, 0 }, { mux_nand_cle, 2, 0 }, { mux_nand_dqs, 2, 0 } },
		{ { mux_nand_re, 2, 0 }, { mux_nand_we, 2, 0 }, { mux_nand_d0, 2, 0 }, { mux_nand_d1, 2, 0 }, { mux_nand_d2, 2, 0 }, { mux_nand_d3, 2, 0 }, { mux_nand_d4, 2, 0 }, { mux_nand_d5, 2, 0 } },
	};
	static const struct {
		int pad;
		char speed;
		char dse;
		char sre;
	} qspi_pad[2][8] = {
		{ { pad_nand_d7, 1, 4, 0 }, { pad_nand_ale, 1, 4, 0 }, { pad_nand_wp, 1, 4, 0 }, { pad_nand_rdy, 1, 4, 0 }, { pad_nand_ce0, 1, 4, 0 }, { pad_nand_ce1, 1, 4, 0 }, { pad_nand_cle, 1, 4, 0 }, { pad_nand_dqs, 1, 4, 0 } },
		{ { pad_nand_re, 1, 4, 0 }, { pad_nand_we, 1, 4, 0 }, { pad_nand_d0, 1, 4, 0 }, { pad_nand_d1, 1, 4, 0 }, { pad_nand_d2, 1, 4, 0 }, { pad_nand_d3, 1, 4, 0 }, { pad_nand_d4, 1, 4, 0 }, { pad_nand_d5, 1, 4, 0 } },
	};

	for (i = 0; i < sizeof(qspi_mux[dev_no]) / sizeof(qspi_mux[dev_no][0]); i++) {
		imx6ull_setIOmux(qspi_mux[dev_no][i].mux, qspi_mux[dev_no][i].sion, qspi_mux[dev_no][i].mode);
	}

	for (i = 0; i < sizeof(qspi_mux[dev_no]) / sizeof(qspi_mux[dev_no][0]); i++) {
		imx6ull_setIOpad(qspi_pad[dev_no][i].pad, 0, 2, 0, 0, 0, qspi_pad[dev_no][i].speed, qspi_pad[dev_no][i].dse, qspi_pad[dev_no][i].sre);
	}
}


__attribute__((section(".noxip"))) static void qspi_enable(int enable)
{
	if (enable == 0) {
		QSPI_BASE[QSPI_MCR] |= QSPI_MCR_CLR_MDIS;
	}
	else {
		QSPI_BASE[QSPI_MCR] &= ~QSPI_MCR_CLR_MDIS;
	}
}


__attribute__((section(".noxip"))) void qspi_lutUpdate(qspi_t *qspi, u32 index, const u32 *lutTable, size_t count)
{
	u32 lutCopy[64];
	size_t i;
	volatile u32 *lutPtr = QSPI_BASE + QSPI_LUT + index;

	(void)qspi;

	/* Copy to prevent BUS error in XIP mode. */
	for (i = 0; i < count; ++i) {
		lutCopy[i] = lutTable[i];
	}

	qspi_enable(0);

	/* Unlock LUT */
	QSPI_BASE[QSPI_LUTKEY] = QSPI_LUTKEY_KEY;
	QSPI_BASE[QSPI_LCKCR] = (QSPI_BASE[QSPI_LCKCR] & ~(QSPI_LCKCR_UNLOCK | QSPI_LCKCR_LOCK)) | QSPI_LCKCR_UNLOCK;

	/* Update LUT */
	for (i = 0; i < count; ++i) {
		lutPtr[i] = lutCopy[i];
	}

	/* Lock LUT */
	QSPI_BASE[QSPI_LUTKEY] = QSPI_LUTKEY_KEY;
	QSPI_BASE[QSPI_LCKCR] = (QSPI_BASE[QSPI_LCKCR] & ~(QSPI_LCKCR_UNLOCK | QSPI_LCKCR_LOCK)) | QSPI_LCKCR_LOCK;

	qspi_enable(1);
}


__attribute__((section(".noxip"))) static void qspi_swReset(void)
{
	QSPI_BASE[QSPI_MCR] |= (QSPI_MCR_SWRSTHD | QSPI_MCR_SWRSTSD);
	qspi_enable(0);
	QSPI_BASE[QSPI_MCR] &= ~(QSPI_MCR_SWRSTHD | QSPI_MCR_SWRSTSD);
	qspi_enable(1);
}


__attribute__((section(".noxip"))) static void qspi_setClk(void)
{
	imx6ull_setDevClock(clk_qspi, 0x03);

	qspi_enable(0);
	/* Set clock source to PLL3 480M Hz with divider 6 = 80 MHz. */
	imx6ull_setQSPIClockSource(CLK_SEL_QSPI1_PLL3, 6);
	qspi_enable(1);
	qspi_swReset();
}


__attribute__((section(".noxip"))) static void qspi_pinConfig(u8 slPortMask)
{
	if ((slPortMask & (qspi_slBusA1 | qspi_slBusA2)) != 0u) {
		qspi_setMux(0);
	}
	if ((slPortMask & (qspi_slBusB1 | qspi_slBusB2)) != 0u) {
		qspi_setMux(1);
	}

	qspi_setClk();
}


void qspi_setFlashSize(qspi_t *qspi, const size_t flashSizes[4])
{
	int i;
	size_t top;

	(void)qspi;

	for (i = 0; i < 4; i++) {
		qspi->slFlashSz[i] = flashSizes[i];
	}

	top = QSPI_MMAP_BASE + flashSizes[0];
	QSPI_BASE[QSPI_SFA1AD] = (QSPI_BASE[QSPI_SFA1AD] & (0x3ffu)) | (top & ~(0x3ffu));
	top += flashSizes[1];
	QSPI_BASE[QSPI_SFA2AD] = (QSPI_BASE[QSPI_SFA2AD] & (0x3ffu)) | (top & ~(0x3ffu));
	top += flashSizes[2];
	QSPI_BASE[QSPI_SFB1AD] = (QSPI_BASE[QSPI_SFB1AD] & (0x3ffu)) | (top & ~(0x3ffu));
	top += flashSizes[3];
	QSPI_BASE[QSPI_SFB2AD] = (QSPI_BASE[QSPI_SFB2AD] & (0x3ffu)) | (top & ~(0x3ffu));
}


__attribute__((section(".noxip"))) static addr_t qspi_getAddressByPort(qspi_t *qspi, u8 port, addr_t addr)
{
	u32 reg;

	/* QSPI use the port (chip select) based on an offset of each memory size */
	switch (port) {
		case 0:
			reg = QSPI_MMAP_BASE;
			break;
		case 1:
			reg = QSPI_BASE[QSPI_SFA1AD];
			break;
		case 2:
			reg = QSPI_BASE[QSPI_SFA2AD];
			break;
		case 3:
			reg = QSPI_BASE[QSPI_SFB1AD];
			break;
		default:
			reg = 0;
			break;
	}
	reg &= ~(0x3ff);

	return addr + reg;
}


int qspi_deinit(qspi_t *qspi)
{
	/* Leaving initialized */
	(void)qspi;

	return EOK;
}


__attribute__((section(".noxip"))) static void qspi_setIPCR(unsigned int seq_num, size_t idatsz)
{
	u32 reg = QSPI_BASE[QSPI_IPCR];

	reg = (reg & ~(0x0fu << 24)) | ((seq_num & 0x0fu) << 24);
	reg = (reg & ~(0xffffu)) | (idatsz & 0xffffu);


	QSPI_BASE[QSPI_SPTRCLR] |= QSPI_SPTRCLR_IP;
	QSPI_BASE[QSPI_IPCR] = reg;
}


__attribute__((section(".noxip"))) static ssize_t qspi_opRead(time_t start, struct xferOp *xfer)
{
	unsigned int i;
	size_t byte, len = 0;
	u32 reg;

	do {
		if ((xfer->timeout > 0u) && (hal_timerGet() - start) >= xfer->timeout) {
			return -ETIME;
		}

		QSPI_BASE[QSPI_RBCT] = ((QSPI_BASE[QSPI_RBCT] & ~(0x1fu)) | WATERMARK);
		QSPI_BASE[QSPI_SFAR] = xfer->addr;

		QSPI_BASE[QSPI_MCR] |= QSPI_MCR_CLR_RXF;

		qspi_setIPCR(xfer->seqIdx, xfer->data.read.sz);
	} while ((QSPI_BASE[QSPI_FR] & (QSPI_FR_IPAEF | QSPI_FR_IPIEF)) != 0u);

	while (len < xfer->data.read.sz) {
		do {
			if ((xfer->timeout > 0u) && (hal_timerGet() - start) >= xfer->timeout) {
				return -ETIME;
			}

			reg = QSPI_BASE[QSPI_SR];
		} while (((reg & QSPI_SR_RXWE) == 0u) && ((reg & QSPI_SR_BUSY) != 0u));

		for (i = 0; (i <= WATERMARK) && (len < xfer->data.read.sz); i++) {
			reg = QSPI_BASE[QSPI_RBDRn(i)];

			for (byte = 0; (byte < 4u) && ((len + byte) < xfer->data.read.sz); byte++) {
				((u8 *)xfer->data.read.ptr)[len + byte] = reg & 0xffu;
				reg >>= 8;
			}
			len += byte;
		}
		QSPI_BASE[QSPI_FR] |= QSPI_FR_RBDF; /* Buffer pop. */
	}

	return len;
}


__attribute__((section(".noxip"))) static size_t qspi_writeTx(const void *data, size_t size)
{
	size_t byte, sent = 0;
	int align64 = 0;
	u32 reg;

	while (((QSPI_BASE[QSPI_SR] & QSPI_SR_TXFULL) == 0u) && (sent < size)) {
		reg = 0;
		for (byte = 0; (byte < 4u) && ((sent + byte) < size); byte++) {
			reg |= ((const u8 *)data)[sent + byte] << (8u * byte);
		}
		sent += byte;

		QSPI_BASE[QSPI_TBDR] = reg;
		align64 += 4;
	}
	for (; ((align64 % 64) != 0) && (QSPI_BASE[QSPI_SR] & QSPI_SR_TXFULL) == 0u; align64 += 4) {
		QSPI_BASE[QSPI_TBDR] = 0;
	}

	return sent;
}


__attribute__((section(".noxip"))) static ssize_t qspi_opWrite(time_t start, struct xferOp *xfer)
{
	size_t sent = 0;

	do {
		QSPI_BASE[QSPI_MCR] |= QSPI_MCR_CLR_TXF;

		QSPI_BASE[QSPI_SFAR] = xfer->addr;

		sent += qspi_writeTx(xfer->data.write.ptr, xfer->data.write.sz);

		qspi_setIPCR(xfer->seqIdx, xfer->data.write.sz);
		if ((xfer->timeout > 0u) && (hal_timerGet() - start) >= xfer->timeout) {
			return -ETIME;
		}
	} while ((QSPI_BASE[QSPI_FR] & (QSPI_FR_IPAEF | QSPI_FR_IPIEF)) != 0u);

	while (sent < xfer->data.write.sz) {
		if ((xfer->timeout > 0u) && (hal_timerGet() - start) >= xfer->timeout) {
			return -ETIME;
		}
		sent += qspi_writeTx(((const u8 *)xfer->data.write.ptr) + sent, xfer->data.write.sz - sent);
	}

	return sent;
}


__attribute__((section(".noxip"))) ssize_t qspi_xferExec(qspi_t *qspi, struct xferOp *xfer)
{
	ssize_t res;
	time_t start = hal_timerGet();

	xfer->addr = qspi_getAddressByPort(qspi, xfer->port, xfer->addr);

	switch (xfer->op) {
		case xfer_opWrite: /* IP write is limited to IPDATSZ mask */
			if (xfer->data.write.sz > 0xffffu) {
				return -EPERM;
			}
			res = qspi_opWrite(start, xfer);
			break;
		case xfer_opRead:
			if (xfer->data.read.sz > 0xffffu) {
				/* Clear buffers. */
				QSPI_BASE[QSPI_MCR] |= (QSPI_MCR_CLR_RXF | QSPI_MCR_CLR_TXF);
				hal_memcpy(xfer->data.read.ptr, (const void *)xfer->addr, xfer->data.read.sz);
				return xfer->data.read.sz;
			}
			res = qspi_opRead(start, xfer);
			break;
		case xfer_opCommand:
			xfer->data.read.sz = 0;
			xfer->data.read.ptr = NULL;
			res = qspi_opRead(start, xfer);
			break;
		default:
			return -EINVAL;
	}

	/* Wait for IP command complete */
	while ((QSPI_BASE[QSPI_SR] & QSPI_SR_BUSY) != 0u) {
		if ((xfer->timeout > 0u) && (hal_timerGet() - start) >= xfer->timeout) {
			QSPI_BASE[QSPI_MCR] |= (QSPI_MCR_CLR_RXF | QSPI_MCR_CLR_TXF);
			return -ETIME;
		}
	}
	QSPI_BASE[QSPI_MCR] |= (QSPI_MCR_CLR_RXF | QSPI_MCR_CLR_TXF);
	return res;
}

__attribute__((section(".noxip"))) int qspi_init(qspi_t *qspi, u8 slPortMask)
{
	unsigned i;
	u32 lut[4];
	static const size_t flashszs[4] = { 4 * 1024 * 1024, 4 * 1024 * 1024, 4 * 1024 * 1024, 4 * 1024 * 1024 };

	if (((slPortMask & ~0xfu) != 0u) || (slPortMask == 0u)) {
		return -EINVAL;
	}

	qspi->ahbAddr = QSPI_MMAP_BASE;
	qspi->slPortMask = slPortMask;

	hal_interruptsDisableAll();

	qspi_pinConfig(slPortMask);

	/* Release QSPI from reset and power SRAM */
	qspi_swReset();
	qspi_enable(0);

	/* Set write to RXBR buffer */
	QSPI_BASE[QSPI_RBCT] |= QSPI_RBCT_RXBRD;

	QSPI_BASE[QSPI_MCR] |= (QSPI_MCR_CLR_RXF | QSPI_MCR_CLR_TXF);

	/* Set temporarily flash size to 4MB */
	qspi_setFlashSize(qspi, flashszs);

	/* Set TCSH and TCSS to one cycle. */
	QSPI_BASE[QSPI_FLSHCR] = (QSPI_BASE[QSPI_FLSHCR] & ~(0x0fu << 8)) | (1u << 8);
	QSPI_BASE[QSPI_FLSHCR] = (QSPI_BASE[QSPI_FLSHCR] & ~(0x0fu)) | 1u;

	/* AHB init */
	/* Set read sequence ID to zero. */
	QSPI_BASE[QSPI_SPTRCLR] |= QSPI_SPTRCLR_AHB;
	QSPI_BASE[QSPI_BFGENCR] &= ~(0xfu << 12);

	/* Only use buffer 3. */
	for (i = 0; i < 3u; i++) {
		QSPI_BASE[QSPI_BUFIND(i)] &= 0x7;
		QSPI_BASE[QSPI_BUFCR(i)] = (((QSPI_BASE[QSPI_BUFCR(i)]) & ~(0xffu << 8)) & ~(0xfu)) | QSPI_BUFCR_INVALID_MASTER;
	}
	/* Set Buffer 3 to handle all masters and have a size of 1KiB. */
	QSPI_BASE[QSPI_BUFCR(3)] = ((QSPI_BASE[QSPI_BUFCR(3)] | 1u << 31) & ~(0xffu << 8)) | (128u << 8);

	/* Default fast (up to 133MHz) read (single pad) used by AHB and XIP */
	lut[0] = LUT_SEQ(lutCmd, lutPad1, 0x0b, lutCmdADDR_SDR, lutPad1, 0x18);
	lut[1] = LUT_SEQ(lutCmdDUMMY, lutPad1, 0x08, lutCmdREAD_SDR, lutPad1, 0x04);
	lut[2] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0);
	lut[3] = 0;

	/* Configure initial LUT sequences as needed (for AHB read and IP) */
	qspi_lutUpdate(qspi, qspi_readData, lut, sizeof(lut) / sizeof(lut[0]));

	qspi_enable(1);

	hal_interruptsEnableAll();

	return EOK;
}
