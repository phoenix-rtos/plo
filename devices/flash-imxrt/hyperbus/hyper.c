/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT hyperflash NOR device driver
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <lib/lib.h>
#include "../lut.h"
#include "../fspi.h"
#include "hyper.h"
#include "hyper_lut.h"


static const struct nor_info hyper_info = { 0, "S29QL512", 64 * 1024 * 1024, 0x200, 0x40000, LUTSZ_HYPER, hyperLut, NULL };


ssize_t hyper_readData(flexspi_t *fspi, addr_t addr, void *data, size_t size, time_t timeout)
{
	struct xferOp xfer;

	xfer.op = xfer_opRead;
	xfer.port = flexspi_slBusA1;
	xfer.timeout = timeout;
	xfer.addr = addr;
	xfer.seqIdx = fspi_hyperReadData;
	xfer.seqNum = 0;
	xfer.data.read.ptr = data;
	xfer.data.read.sz = size;

	return flexspi_xferExec(fspi, &xfer);
}


ssize_t hyper_writeData(flexspi_t *fspi, addr_t addr, const void *data, size_t size, time_t timeout)
{
	struct xferOp xfer;

	xfer.op = xfer_opWrite;
	xfer.port = flexspi_slBusA1;
	xfer.timeout = timeout;
	xfer.addr = addr;
	xfer.seqIdx = fspi_hyperWriteData;
	xfer.seqNum = 0;
	xfer.data.write.ptr = data;
	xfer.data.write.sz = size;

	return flexspi_xferExec(fspi, &xfer);
}


int hyper_writeEnable(flexspi_t *fspi, addr_t addr, time_t timeout)
{
	struct xferOp xfer;

	xfer.op = xfer_opCommand;
	xfer.port = flexspi_slBusA1;
	xfer.timeout = timeout;
	xfer.addr = addr;
	xfer.seqIdx = fspi_hyperWriteEnable;
	xfer.seqNum = 1;

	return flexspi_xferExec(fspi, &xfer);
}


int hyper_readStatus(flexspi_t *fspi, u16 *statusWord, time_t timeout)
{
	struct xferOp xfer;

	xfer.op = xfer_opRead;
	xfer.port = flexspi_slBusA1;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = fspi_hyperReadStatus;
	xfer.seqNum = 1;
	xfer.data.read.ptr = (u8 *)statusWord;
	xfer.data.read.sz = 2;

	return flexspi_xferExec(fspi, &xfer);
}


int hyper_waitBusy(flexspi_t *fspi, time_t timeout)
{
	int res;
	u16 status = 0;

	do {
		res = hyper_readStatus(fspi, &status, timeout);
		if (res < EOK) {
			return res;
		}

		if ((status & ((3 << 12) | (1 << 9))) != 0) {
			return -ETIME;
		}
	} while ((status & (1 << 15)) != 0);

	return EOK;
}


int hyper_eraseSector(flexspi_t *fspi, addr_t addr, time_t timeout)
{
	struct xferOp xfer;

	int res = hyper_writeEnable(fspi, addr, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opCommand;
	xfer.port = flexspi_slBusA1;
	xfer.timeout = timeout;
	xfer.addr = addr;
	xfer.seqIdx = fspi_hyperEraseSector;
	xfer.seqNum = 3;

	res = flexspi_xferExec(fspi, &xfer);
	if (res < EOK) {
		return res;
	}

	return hyper_waitBusy(fspi, timeout);
}


int hyper_pageProgram(flexspi_t *fspi, addr_t dstAddr, const void *src, size_t pageSz, time_t timeout)
{
	struct xferOp xfer;

	int res = hyper_writeEnable(fspi, dstAddr, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opWrite;
	xfer.port = flexspi_slBusA1;
	xfer.timeout = timeout;
	xfer.addr = dstAddr;
	xfer.seqIdx = fspi_hyperPageProgram;
	xfer.seqNum = 1;
	xfer.data.write.ptr = src;
	xfer.data.write.sz = pageSz;

	res = flexspi_xferExec(fspi, &xfer);
	if (res < EOK) {
		return res;
	}

	return hyper_waitBusy(fspi, timeout);
}


int hyper_probe(flexspi_t *fspi, const struct nor_info **pInfo, time_t timeout)
{
	int res;
	u32 buf[2];
	size_t flashSz;

	buf[0] = 0x9800;
	res = hyper_writeData(fspi, 0x555, &buf[0], 2, timeout);
	if (res < EOK) {
		return res;
	}

	/* Read ID-CFI */
	res = hyper_readData(fspi, 0x10, &buf[0], sizeof(buf), timeout);
	if (res < EOK) {
		return res;
	}

	buf[1] &= 0xffff;
	/* Check string "QRY" (unicode in big-endian order) */
	if (buf[0] != 0x52005100 || buf[1] != 0x5900) {
		return -ENXIO; /* not found */
	}

	buf[0] = 0xf000;
	res = hyper_writeData(fspi, 0, &buf[0], 2, timeout);
	if (res < EOK) {
		return res;
	}

	if (pInfo) {
		*pInfo = &hyper_info;
	}

	flexspi_lutUpdate(fspi, 0, hyperLut, sizeof(hyperLut) / sizeof(hyperLut[0]));
	flexspi_setFlashSize(fspi, &flashSz, 1);

	/* Found hyperflash device */
	return EOK;
}
