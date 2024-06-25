/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT nor flash device driver
 * ISSI Specific
 *
 * Copyright 2024 Phoenix Systems
 * Authors: Ziemowit Leszczynski
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


int nor_issiInit(struct nor_device *dev)
{
	int res;
	struct xferOp xfer;
	u8 status, params;
	const time_t timeout = 1000;

	if (dev->active == 0) {
		return -ENODEV;
	}

	/* Load only the necessary commands to enable quad mode and set dummy cycles. */
	flexspi_lutUpdateEntries(&dev->fspi, fspi_readStatus * LUT_SEQSZ, dev->nor->lut + fspi_readStatus, 1, LUT_SEQSZ);
	flexspi_lutUpdateEntries(&dev->fspi, fspi_writeStatus * LUT_SEQSZ, dev->nor->lut + fspi_writeStatus, 1, LUT_SEQSZ);
	flexspi_lutUpdateEntries(&dev->fspi, fspi_writeEnable * LUT_SEQSZ, dev->nor->lut + fspi_writeEnable, 1, LUT_SEQSZ);
	flexspi_lutUpdateEntries(&dev->fspi, fspi_cmdCustom1 * LUT_SEQSZ, dev->nor->lut + fspi_cmdCustom1, 1, LUT_SEQSZ);
	flexspi_lutUpdateEntries(&dev->fspi, fspi_cmdCustom2 * LUT_SEQSZ, dev->nor->lut + fspi_cmdCustom2, 1, LUT_SEQSZ);

	res = nor_readStatus(&dev->fspi, dev->port, &status, timeout);
	if (res < EOK) {
		return res;
	}

	if ((status & (1uL << 6u)) == 0uL) {
		res = nor_writeEnable(&dev->fspi, dev->port, 1, timeout);
		if (res < EOK) {
			return res;
		}

		/* Quad Enable */
		status |= (1uL << 6u);

		xfer.op = xfer_opWrite;
		xfer.port = dev->port;
		xfer.timeout = timeout;
		xfer.addr = 0;
		xfer.seqIdx = LUT_SEQIDX(fspi_writeStatus);
		xfer.seqNum = LUT_SEQNUM(fspi_writeStatus);
		xfer.data.write.ptr = &status;
		xfer.data.write.sz = 1;

		res = flexspi_xferExec(&dev->fspi, &xfer);
		if (res < EOK) {
			return res;
		}

		res = nor_waitBusy(&dev->fspi, dev->port, timeout);
		if (res < EOK) {
			return res;
		}
	}

	xfer.op = xfer_opRead;
	xfer.port = dev->port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(fspi_cmdCustom1);
	xfer.seqNum = LUT_SEQNUM(fspi_cmdCustom1);
	xfer.data.read.ptr = &params;
	xfer.data.read.sz = 1;

	res = flexspi_xferExec(&dev->fspi, &xfer);
	if (res < EOK) {
		return res;
	}

	if (((params & 0x87) >> 3u) != 11) {
		/* 11 dummy cycles */
		params = (params & 0x87) | (11uL << 3u);

		xfer.op = xfer_opWrite;
		xfer.port = dev->port;
		xfer.timeout = timeout;
		xfer.addr = 0;
		xfer.seqIdx = LUT_SEQIDX(fspi_cmdCustom2);
		xfer.seqNum = LUT_SEQNUM(fspi_cmdCustom2);
		xfer.data.read.ptr = &params;
		xfer.data.write.sz = 1;

		res = flexspi_xferExec(&dev->fspi, &xfer);
		if (res < EOK) {
			return res;
		}

		res = nor_waitBusy(&dev->fspi, dev->port, timeout);
		if (res < EOK) {
			return res;
		}
	}

	return EOK;
}
