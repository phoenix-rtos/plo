/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT nor flash device driver
 * Macronix Specific
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <lib/lib.h>
#include <devices/devs.h>
#include "../fspi.h"
#include "../lut.h"

#include "flash.h"
#include "nor.h"


int nor_mxQuadEnable(struct nor_device *dev)
{
	int res;
	struct xferOp xfer;
	u32 quadEnable = 0x40;
	const time_t timeout = 1000;

	if (!dev->active) {
		return -ENODEV;
	}

	/*
	 * Load only the necessary commands, let assume that the LUT table has only one
	 * entry for XIP (legacy read in single mode) at `fspi_readData` location, than
	 * can safely load commands while XIP or AHB any read.
	 */
	flexspi_lutUpdate(&dev->fspi, fspi_writeEnable * 4, dev->nor->lut + fspi_writeEnable * 4, 4);
	flexspi_lutUpdate(&dev->fspi, fspi_writeStatus * 4, dev->nor->lut + fspi_writeStatus * 4, 4);
	flexspi_lutUpdate(&dev->fspi, fspi_readStatus * 4, dev->nor->lut + fspi_readStatus * 4, 4);

	xfer.op = xfer_opRead;
	xfer.port = dev->port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(fspi_readStatus);
	xfer.seqNum = LUT_SEQNUM(fspi_readStatus);
	xfer.data.read.ptr = &quadEnable;
	xfer.data.read.sz = 1;

	res = flexspi_xferExec(&dev->fspi, &xfer);
	if (res < EOK) {
		return res;
	}

	if (quadEnable & (1 << 6)) {
		/* Already enabled */
		return EOK;
	}

	quadEnable |= (1 << 6);

	res = nor_writeEnable(&dev->fspi, dev->port, 1, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opWrite;
	xfer.port = dev->port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(fspi_writeStatus);
	xfer.seqNum = LUT_SEQNUM(fspi_writeStatus);
	xfer.data.write.ptr = &quadEnable;
	xfer.data.write.sz = 1;

	res = flexspi_xferExec(&dev->fspi, &xfer);
	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(&dev->fspi, dev->port, timeout);
}
