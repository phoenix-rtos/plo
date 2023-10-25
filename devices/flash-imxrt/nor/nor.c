/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT NOR flash device driver
 *
 * Copyright 2021-2023 Phoenix Systems
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
#include "nor_lut.h"


#define FLASH_ID(vid, pid) ((vid & 0xff) | (pid & 0xff00) | ((pid & 0xff) << 16))


static const char *nor_vendors[] = {
	"\xef Winbond",
	"\x20 Micron",
	"\x9d ISSI",
	"\xc2 Macronix",
	NULL
};


static const struct nor_info flashInfo[] = {
	/* Winbond */
	{ FLASH_ID(0xef, 0x4015), "W25Q16", 2 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric3Byte, NULL },
	{ FLASH_ID(0xef, 0x4016), "W25Q32", 4 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric3Byte, NULL },
	{ FLASH_ID(0xef, 0x4017), "W25Q64", 8 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric3Byte, NULL },
	{ FLASH_ID(0xef, 0x4018), "W25Q128", 16 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric3Byte, NULL },
	{ FLASH_ID(0xef, 0x4019), "W25Q256JVEIQ", 32 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric4Byte, NULL },
	{ FLASH_ID(0xef, 0x6019), "W25Q256JW-Q", 32 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric4Byte, NULL },
	{ FLASH_ID(0xef, 0x8019), "W25Q256JW-M", 32 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric4Byte, NULL },

	/* ISSI */
	{ FLASH_ID(0x9d, 0x7016), "IS25WP032", 4 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric3Byte, NULL },
	{ FLASH_ID(0x9d, 0x7017), "IS25WP064", 8 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric3Byte, NULL },
	{ FLASH_ID(0x9d, 0x7018), "IS25WP128", 16 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric3Byte, NULL },

	/* Micron */
	{ FLASH_ID(0x20, 0xba19), "MT25QL256", 32 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC | NOR_CAPS_EN4B, lutMicronMono, NULL },
	{ FLASH_ID(0x20, 0xba20), "MT25QL512", 64 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC | NOR_CAPS_EN4B, lutMicronMono, NULL },
	{ FLASH_ID(0x20, 0xba21), "MT25QL01G", 128 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_DIE2 | NOR_CAPS_EN4B, lutMicronDie, NULL },
	{ FLASH_ID(0x20, 0xba22), "MT25QL02G", 256 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_DIE4 | NOR_CAPS_EN4B, lutMicronDie, NULL },

	/* Macronix (MXIX) */
	{ FLASH_ID(0xc2, 0x2016), "MX25L3233", 4 * 1024 * 1024, 0x100, 0x1000, NOR_CAPS_GENERIC, lutGeneric3Byte, nor_mxQuadEnable },
};


static int nor_readID(flexspi_t *fspi, u8 port, u32 *retValue, time_t timeout)
{
	struct xferOp xfer;
	u32 lut[4] = { LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_RDID, lutCmdREAD_SDR, lutPad1, 0x04), 0, 0, 0 };

	xfer.op = xfer_opRead;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(fspi_readID);
	xfer.seqNum = LUT_SEQNUM(fspi_readID);
	xfer.data.read.ptr = retValue;
	xfer.data.read.sz = 3;

	flexspi_lutUpdate(fspi, fspi_readID * NOR_LUTSEQSZ, lut, NOR_LUTSEQSZ);

	return flexspi_xferExec(fspi, &xfer);
}


__attribute__((section(".noxip"))) int nor_readStatus(flexspi_t *fspi, u8 port, u8 *statusByte, time_t timeout)
{
	struct xferOp xfer;

	xfer.op = xfer_opRead;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(fspi_readStatus);
	xfer.seqNum = LUT_SEQNUM(fspi_readStatus);
	xfer.data.read.ptr = statusByte;
	xfer.data.read.sz = 1;

	return flexspi_xferExec(fspi, &xfer);
}


__attribute__((section(".noxip"))) int nor_waitBusy(flexspi_t *fspi, u8 port, time_t timeout)
{
	int res;
	u8 status = 0;

	do {
		res = nor_readStatus(fspi, port, &status, timeout);
		if (res < EOK) {
			return res;
		}
	} while ((status & 1) != 0);

	return EOK;
}


__attribute__((section(".noxip"))) int nor_writeEnable(flexspi_t *fspi, u8 port, int enable, time_t timeout)
{
	struct xferOp xfer;
	int res, seqCode;
	u8 status;

	res = nor_waitBusy(fspi, port, timeout);
	if (res < EOK) {
		return res;
	}

	seqCode = (enable ? fspi_writeEnable : fspi_writeDisable);

	xfer.op = xfer_opCommand;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(seqCode);
	xfer.seqNum = LUT_SEQNUM(seqCode);

	res = flexspi_xferExec(fspi, &xfer);
	if (res < EOK) {
		return res;
	}

	res = nor_readStatus(fspi, port, &status, timeout);
	if (res < EOK) {
		return res;
	}

	if (((status >> 1) & 1) != !!enable) {
		return -EACCES;
	}

	return EOK;
}


__attribute__((section(".noxip"))) int nor_eraseSector(flexspi_t *fspi, u8 port, addr_t addr, time_t timeout)
{
	struct xferOp xfer;

	int res = nor_writeEnable(fspi, port, 1, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opCommand;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = addr;
	xfer.seqIdx = LUT_SEQIDX(fspi_eraseSector);
	xfer.seqNum = LUT_SEQNUM(fspi_eraseSector);

	res = flexspi_xferExec(fspi, &xfer);
	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(fspi, port, timeout);
}


__attribute__((section(".noxip"))) static int nor_mode4ByteAddr(flexspi_t *fspi, u8 port, int en4b, time_t timeout)
{
	struct xferOp xfer;
	int seqCode = ((en4b == 0) ? fspi_exit4byteAddr : fspi_enter4byteAddr);

	xfer.op = xfer_opCommand;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(seqCode);
	xfer.seqNum = LUT_SEQNUM(seqCode);

	return flexspi_xferExec(fspi, &xfer);
}


__attribute__((section(".noxip"))) int nor_eraseChipDie(flexspi_t *fspi, u8 port, u32 capFlags, int dieCount, size_t dieSize, time_t timeout)
{
	struct xferOp xfer;
	int dieIndex, res = -EIO;

	for (dieIndex = 0; dieIndex < dieCount; ++dieIndex) {
		res = nor_writeEnable(fspi, port, 1, timeout);
		if (res < EOK) {
			break;
		}

		if ((dieCount > 1) && ((capFlags & NOR_CAPS_EN4B) != 0u)) {
			res = nor_mode4ByteAddr(fspi, port, 1, timeout);
			if (res < EOK) {
				break;
			}
		}

		xfer.op = xfer_opCommand;
		xfer.port = port;
		xfer.timeout = timeout;
		xfer.addr = dieIndex * dieSize;
		xfer.seqIdx = LUT_SEQIDX(fspi_eraseChip);
		xfer.seqNum = LUT_SEQNUM(fspi_eraseChip);

		res = flexspi_xferExec(fspi, &xfer);
		if (res < EOK) {
			break;
		}

		res = nor_waitBusy(fspi, port, timeout);
		if (res < EOK) {
			break;
		}
	}

	return res;
}


__attribute__((section(".noxip"))) int nor_pageProgram(flexspi_t *fspi, u8 port, addr_t dstAddr, const void *src, size_t pageSz, time_t timeout)
{
	struct xferOp xfer;

	int res = nor_writeEnable(fspi, port, 1, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opWrite;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = dstAddr;
	xfer.seqIdx = LUT_SEQIDX(fspi_programQPP);
	xfer.seqNum = LUT_SEQNUM(fspi_programQPP);
	xfer.data.write.ptr = src;
	xfer.data.write.sz = pageSz;

	res = flexspi_xferExec(fspi, &xfer);

	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(fspi, port, timeout);
}


ssize_t nor_readData(flexspi_t *fspi, u8 port, addr_t addr, void *data, size_t size, time_t timeout)
{
	struct xferOp xfer;

	xfer.op = xfer_opRead;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = addr;
	xfer.seqIdx = LUT_SEQIDX(fspi_readData);
	xfer.seqNum = LUT_SEQNUM(fspi_readData);
	xfer.data.read.ptr = data;
	xfer.data.read.sz = size;

	return flexspi_xferExec(fspi, &xfer);
}


int nor_probe(flexspi_t *fspi, u8 port, const struct nor_info **pInfo, const char **pVendor)
{
	int res, i;
	u32 jedecId = 0;

	res = nor_readID(fspi, port, &jedecId, 1000);
	if (res < EOK) {
		return res;
	}

	res = -ENXIO;

	lib_printf("\ndev/flash/nor: Probing flash id 0x%08x", jedecId);

	for (i = 0; i < sizeof(flashInfo) / sizeof(flashInfo[0]); ++i) {
		if (flashInfo[i].jedecId == jedecId) {
			res = EOK;
			if (pInfo != NULL) {
				*pInfo = &flashInfo[i];
			}

			break;
		}
	}

	if (res == EOK && pVendor != NULL) {
		*pVendor = "Unknown";

		for (i = 0; nor_vendors[i]; ++i) {
			if (*nor_vendors[i] == (jedecId & 0xff)) {
				*pVendor = &nor_vendors[i][2];
				break;
			}
		}
	}

	return res;
}


void nor_deviceInit(struct nor_device *dev, int port, int active, time_t timeout)
{
	dev->nor = NULL;
	dev->port = port;
	dev->active = active;
	dev->timeout = timeout;
	dev->sectorPrevAddr = (addr_t)-1;
	dev->sectorSyncAddr = (addr_t)-1;

	hal_memset(dev->sectorBuf, NOR_ERASED_STATE, sizeof(dev->sectorBuf));
}
