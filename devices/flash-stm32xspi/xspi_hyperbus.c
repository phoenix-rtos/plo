/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * STM32 XSPI HyperBus RAM driver
 *
 * Copyright 2020, 2025 Phoenix Systems
 * Author: Krzysztof Radzewicz, Jacek Maksymowicz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "xspi_common.h"

#define XSPI_CCR_DQSE         (1UL << 29) /* DQS (data strobe) enable */
#define XSPI_CCR_DDTR         (1UL << 27) /* Data double transfer rate. Always on in HyperBus */
#define XSPI_CCR_DMODE_16BIT  (5UL << 24)
#define XSPI_CCR_DMODE_MASK   (7UL << 24)
#define XSPI_CCR_ADMODE_MASK  (7UL << 8)
#define XSPI_CCR_ADMODE_SHIFT 8UL

#define XSPI_HLCR_TRWR_MASK  (0xffUL << 16) /* Read-write minimum recovery time */
#define XSPI_HLCR_TRWR_SHIFT 16UL
#define XSPI_HLCR_TACC_MASK  (0xffUL << 8) /* Access time */
#define XSPI_HLCR_TACC_SHIFT 8UL
#define XSPI_HLCR_WZL        (1UL << 1) /* Write zero latency */
#define XSPI_HLCR_LM         (1UL << 0) /* Latency mode (fixed/variable) */

#define HYPERBUS_IR0 0x0UL
#define HYPERBUS_IR1 0x1UL
#define HYPERRAM_CR0 0xa00UL
#define HYPERRAM_CR1 0xa01UL

#define HYPERBUS_TYPE_RAM  0x0UL /* HYPERRAM™ */
#define HYPERBUS_TYPE_RAM2 0x1UL /* HYPERRAM™ 2.0 */
#define HYPERBUS_TYPE_RAM3 0x9UL /* HYPERRAM™ 3.0 a.k.a. HYPERRAM™ extended-IO */


/* XSPI registers needed for configuring a HyperBus transaction */
typedef struct {
	u32 cr;
	u32 dcr1;
	u32 ccr;
	u32 hlcr;
	u32 wccr;
} psram_xspiSetup_t;

/* Bits to be cleared with the existing register configuration */
const static psram_xspiSetup_t psram_toClear = {
	.cr = XSPI_CR_MODE_MASK,
	.dcr1 = XSPI_DCR1_MTYP_MASK,
	.ccr = XSPI_CCR_DQSE |
			XSPI_CCR_DDTR |
			XSPI_CCR_ADMODE_MASK,
	.hlcr = XSPI_HLCR_TRWR_MASK |
			XSPI_HLCR_TACC_MASK |
			XSPI_HLCR_WZL |
			XSPI_HLCR_LM,
	.wccr = XSPI_CCR_DQSE |
			XSPI_CCR_DDTR |
			XSPI_CCR_ADMODE_MASK,
};

/* XSPI configurations for HyperBus transactions. For now only indirect mode */
typedef enum psram_xspiMode_t {
	xspi_memRead,
	xspi_regRead,
	xspi_memWrite,
	xspi_regWrite,
	xspi_memMapped,
	xspi_modeCount,
} psram_xspiMode_t;

/* Bits to be OR'd to the existing register configuration  */
static const psram_xspiSetup_t psram_xspiModes[xspi_modeCount] = {
	[xspi_memRead] = {
		.cr = XSPI_CR_MODE_IREAD,
		.dcr1 = XSPI_DCR1_MTYP_HBUS_MEM,
		.ccr = (4 << XSPI_CCR_ADMODE_SHIFT) | XSPI_CCR_DDTR | XSPI_CCR_DQSE,
		.hlcr = (7 << XSPI_HLCR_TRWR_SHIFT) | (7 << XSPI_HLCR_TACC_SHIFT),
		.wccr = 0,
	},
	[xspi_regRead] = {
		.cr = XSPI_CR_MODE_IREAD,
		.dcr1 = XSPI_DCR1_MTYP_HBUS_REG,
		.ccr = (4 << XSPI_CCR_ADMODE_SHIFT) | XSPI_CCR_DDTR | XSPI_CCR_DQSE,
		.hlcr = (7 << XSPI_HLCR_TRWR_SHIFT) | (7 << XSPI_HLCR_TACC_SHIFT),
		.wccr = 0,
	},
	[xspi_memWrite] = {
		.cr = XSPI_CR_MODE_IWRITE,
		.dcr1 = XSPI_DCR1_MTYP_HBUS_MEM,
		.ccr = XSPI_CCR_DQSE | XSPI_CCR_DDTR,
		.hlcr = (7 << XSPI_HLCR_TRWR_SHIFT) | (7 << XSPI_HLCR_TACC_SHIFT),
		.wccr = 0,
	},
	[xspi_regWrite] = {
		.cr = XSPI_CR_MODE_IWRITE,
		.dcr1 = XSPI_DCR1_MTYP_HBUS_REG,
		.ccr = 0,
		.hlcr = (XSPI_HLCR_WZL) | (XSPI_HLCR_LM),
		.wccr = 0,
	},
	[xspi_memMapped] = {
		.cr = XSPI_CR_MODE_MEMORY,
		.dcr1 = XSPI_DCR1_MTYP_HBUS_MEM,
		.ccr = (4 << XSPI_CCR_ADMODE_SHIFT) | XSPI_CCR_DDTR | XSPI_CCR_DQSE,
		.hlcr = (7 << XSPI_HLCR_TRWR_SHIFT) | (7 << XSPI_HLCR_TACC_SHIFT),
		.wccr = XSPI_CCR_DDTR | XSPI_CCR_DQSE,
	},
};

/* Parameters of memory connected to a controller */
static struct hb_memParams {
	psram_xspiMode_t memMode;
	u8 is16Bit;
	u8 isRAM;
	u8 log_size;
} hb_memParams[XSPI_N_CONTROLLERS];


static int psramdrv_changeXspiMode(unsigned int minor, psram_xspiMode_t mode)
{
	const psram_xspiSetup_t *newSetup = &psram_xspiModes[mode];
	const psram_xspiSetup_t *toClear = &psram_toClear;
	const xspi_ctrlParams_t *p = &xspi_ctrlParams[minor];
	u32 v, prev_mode;

	if (mode == hb_memParams[minor].memMode) {
		return EOK;
	}

	v = *(p->ctrl + xspi_cr);
	prev_mode = (v & XSPI_CR_MODE_MASK);
	if ((prev_mode == XSPI_CR_MODE_MEMORY) || (prev_mode == XSPI_CR_MODE_AUTOPOLL)) {
		hal_cpuDataMemoryBarrier();
		*(p->ctrl + xspi_cr) = v | (1 << 1); /* Abort operation in progress */
		xspi_waitBusy(minor);
	}

	*(p->ctrl + xspi_cr) = (v & ~(toClear->cr)) | newSetup->cr;
	*(p->ctrl + xspi_dcr1) = (*(p->ctrl + xspi_dcr1) & ~(toClear->dcr1)) | newSetup->dcr1;
	*(p->ctrl + xspi_ccr) = (*(p->ctrl + xspi_ccr) & ~(toClear->ccr)) | newSetup->ccr;
	*(p->ctrl + xspi_hlcr) = (*(p->ctrl + xspi_hlcr) & ~(toClear->hlcr)) | newSetup->hlcr;
	*(p->ctrl + xspi_wccr) = (*(p->ctrl + xspi_wccr) & ~(toClear->wccr)) | newSetup->wccr;

	xspi_waitBusy(minor);
	hb_memParams[minor].memMode = mode;

	return EOK;
}


static int xspi_hb_transactionInternal(unsigned int minor, const psram_xspiMode_t mode, u32 sysaddr, u8 *data, u32 size)
{
	const xspi_ctrlParams_t *p = &xspi_ctrlParams[minor];
	u8 isRead = ((mode == xspi_memRead) || (mode == xspi_regRead)) ? 1 : 0;

	psramdrv_changeXspiMode(minor, mode);
	*(p->ctrl + xspi_dlr) = size - 1;
	hal_cpuDataMemoryBarrier();
	*(p->ctrl + xspi_ar) = sysaddr * ((hb_memParams[minor].is16Bit != 0) ? 4 : 2);
	hal_cpuDataMemoryBarrier();

	return xspi_transferFifo(minor, data, size, isRead);
}


static int xspi_hb_detectHyperbusDev(unsigned int minor)
{
	u16 idReg0, idReg1;
	u8 dev_type;
	int ret;
	ret = xspi_hb_transactionInternal(minor, xspi_regRead, HYPERBUS_IR1, (u8 *)&idReg1, 2);
	if (ret < 0) {
		return ret;
	}

	dev_type = idReg1 & 0xf;

	/* HYPERBUS_TYPE_RAM and HYPERBUS_TYPE_RAM2 may also work, but have not been tested.
	 * For now exit with error if it happens. */
	if (dev_type != HYPERBUS_TYPE_RAM3) {
		return -EINVAL;
	}

	if (dev_type == HYPERBUS_TYPE_RAM3) {
		hb_memParams[minor].isRAM = 1;
		hb_memParams[minor].is16Bit = 1;
	}
	else {
		hb_memParams[minor].is16Bit = 0;
		if ((dev_type == HYPERBUS_TYPE_RAM) || (dev_type == HYPERBUS_TYPE_RAM2)) {
			hb_memParams[minor].isRAM = 1;
		}
		else {
			hb_memParams[minor].isRAM = 0;
		}
	}

	ret = xspi_hb_transactionInternal(minor, xspi_regRead, HYPERBUS_IR0, (u8 *)&idReg0, 2);
	if (ret < 0) {
		return ret;
	}
	hb_memParams[minor].log_size =
			((idReg0 >> 8) & 0x1f) +                       /* Row address bits - 1 */
			((idReg0 >> 4) & 0xf) +                        /* Column address bits - 1 */
			((hb_memParams[minor].is16Bit != 0) ? 2 : 1) + /* log2 of word size */
			2;

	return EOK;
}


static int xspi_hb_psramInit(unsigned int minor)
{
	u32 cr0 = 0;
	int ret;

	ret = xspi_hb_transactionInternal(minor, xspi_regRead, HYPERRAM_CR0, (u8 *)&cr0, 4);
	if (ret < 0) {
		return ret;
	}

	cr0 = cr0 & ~0x8; /* Enable variable latency for better performance */
	ret = xspi_hb_transactionInternal(minor, xspi_regWrite, HYPERRAM_CR0, (u8 *)&cr0, 4);
	if (ret < 0) {
		return ret;
	}

	/* Set device to memory mapped mode */
	psramdrv_changeXspiMode(minor, xspi_memMapped);
	return EOK;
}


int xspi_hb_sync(unsigned int minor)
{
	/* TODO: it may be faster to clear the whole DCache instead of doing it by address
	 * or we can disable caching of this region using MPU */
	hal_cpuInvCache(hal_cpuDCache, (addr_t)xspi_ctrlParams[minor].start, xspi_memSize[minor]);
	return EOK;
}


ssize_t xspi_hb_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	(void)timeout;
	hal_memcpy(buff, xspi_ctrlParams[minor].start + offs, len);
	return (ssize_t)len;
}


ssize_t xspi_hb_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	if (hb_memParams[minor].isRAM != 0) {
		hal_memcpy(xspi_ctrlParams[minor].start + offs, buff, len);
		return (ssize_t)len;
	}
	else {
		/* Flash not currently supported */
		return -EINVAL;
	}
}


ssize_t xspi_hb_erase(unsigned int minor, addr_t offs, size_t len, unsigned int flags)
{
	if (hb_memParams[minor].isRAM != 0) {
		if (len == (size_t)-1) {
			len = xspi_memSize[minor];
		}

		hal_memset(xspi_ctrlParams[minor].start + offs, 0, len);
		return (ssize_t)len;
	}
	else {
		/* Flash not currently supported */
		return -EINVAL;
	}
}


size_t xspi_hb_getBlockSize(unsigned int minor)
{
	if (hb_memParams[minor].isRAM != 0) {
		return 1;
	}
	else {
		/* Flash not currently supported */
		return 0;
	}
}


int xspi_hb_init(unsigned int minor)
{
	int ret;
	u32 v;
	const xspi_ctrlParams_t *p = &xspi_ctrlParams[minor];
	struct hb_memParams *mp = &hb_memParams[minor];

	mp->memMode = xspi_modeCount; /* Initially set to invalid mode */
	mp->log_size = 0;
	*(p->ctrl + xspi_dcr1) =
			XSPI_CR_MODE_IWRITE |     /* Indirect write mode */
			XSPI_DCR1_MTYP_HBUS_REG | /* HyperBus register mode */
			(23 << 16) |              /* 16 MB size (will be updated later) */
			(0 << 8) |                /* Chip-select high for at least 1 cycle */
			(0 << 1);                 /* Free-running clock disable */

	*(p->ctrl + xspi_ccr) =
			XSPI_CCR_DQSE | /* Enable data strobe line */
			XSPI_CCR_DDTR;  /* Data double transfer rate */

	/* Memory mapped mode configured later */
	*(p->ctrl + xspi_wccr) = 0;
	*(p->ctrl + xspi_wtcr) = 0;
	*(p->ctrl + xspi_wir) = 0;

	/* Enable controller */
	*(p->ctrl + xspi_cr) |= 1;
	xspi_waitBusy(minor);

	ret = xspi_hb_detectHyperbusDev(minor);
	if (ret < 0) {
		return -EINVAL;
	}

	/* If memory is this small (16 bytes or less), autodetection probably failed. */
	if (mp->log_size <= 4) {
		return -EINVAL;
	}

	/* xspi_memSize only holds 32-bit numbers */
	if (mp->log_size > 31) {
		mp->log_size = 31;
	}

	xspi_memSize[minor] = 1UL << mp->log_size;
	/* Limit size of the device to what's accessible */
	if (xspi_memSize[minor] > xspi_ctrlParams[minor].size) {
		xspi_memSize[minor] = xspi_ctrlParams[minor].size;
	}

	/* Write actual memory size to DCR1 */
	v = *(p->ctrl + xspi_dcr1) & ~(0x1f << 16);
	*(p->ctrl + xspi_dcr1) = v | ((mp->log_size - 1) << 16);

	if (mp->is16Bit != 0) {
		*(p->ctrl + xspi_ccr) |= XSPI_CCR_DMODE_16BIT;
		*(p->ctrl + xspi_wccr) |= XSPI_CCR_DMODE_16BIT;
	}

	xspi_setHigherClock(minor);

	if (mp->isRAM != 0) {
		ret = xspi_hb_psramInit(minor);
		if (ret < 0) {
			return ret;
		}

		lib_printf("\ndev/psram: Configured HYPERRAM %dMB (%d.%d), addr 0x%p",
				xspi_memSize[minor] >> 20,
				DEV_STORAGE,
				minor,
				p->start);
		return EOK;
	}

	/* Non-RAM HyperBus devices currently not supported */
	return -EINVAL;
}
