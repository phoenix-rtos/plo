/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ULL nor flash device driver
 * Macronix Specific
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <lib/lib.h>
#include <devices/devs.h>
#include "../qspi.h"
#include "../lut.h"

#include "flash.h"
#include "nor.h"


int nor_mxQuadEnable(struct nor_device *dev)
{
	int res;
	struct xferOp xfer;
	u32 quadEnable = 0x40;
	const time_t timeout = 1000;

	if (dev->active == 0) {
		return -ENODEV;
	}

	/*
	 * Load only the necessary commands, let assume that the LUT table has only one
	 * entry for XIP (legacy read in single mode) at `qspi_readData` location, than
	 * can safely load commands while XIP or AHB any read.
	 */
	qspi_lutUpdate(&dev->qspi, qspi_writeEnable * 4, dev->nor->lut + qspi_writeEnable * 4, 4);
	qspi_lutUpdate(&dev->qspi, qspi_writeStatus * 4, dev->nor->lut + qspi_writeStatus * 4, 4);
	qspi_lutUpdate(&dev->qspi, qspi_readStatus * 4, dev->nor->lut + qspi_readStatus * 4, 4);

	xfer.op = xfer_opRead;
	xfer.port = dev->port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(qspi_readStatus);
	xfer.data.read.ptr = &quadEnable;
	xfer.data.read.sz = 1;

	res = qspi_xferExec(&dev->qspi, &xfer);
	if (res < EOK) {
		return res;
	}

	if ((quadEnable & (1u << 6)) != 0u) {
		/* Already enabled */
		return EOK;
	}

	quadEnable |= (1u << 6);

	res = nor_writeEnable(&dev->qspi, dev->port, 1, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opWrite;
	xfer.port = dev->port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(qspi_writeStatus);
	xfer.data.write.ptr = &quadEnable;
	xfer.data.write.sz = 1;

	res = qspi_xferExec(&dev->qspi, &xfer);
	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(&dev->qspi, dev->port, timeout);
}
