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
#include "../qspi.h"
#include "../lut.h"

#include "flash.h"
#include "nor.h"
#include "nor_lut.h"

/* clang-format off */
#define FLASH_ID(vid, pid) (((vid) & 0xffu) | ((pid) & 0xff00u) | (((pid) & 0xffu) << 16))


static const char *nor_vendors[] = {
	"\xef" " Winbond",
	"\x20" " Micron",
	"\x9d" " ISSI",
	"\xc2" " Macronix",
	NULL
};
/* clang-format on */


static const struct nor_info flashInfo[] = {
	/* Winbond */
	{ FLASH_ID(0xefu, 0x4015u), "W25Q16", 2 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },
	{ FLASH_ID(0xefu, 0x4016u), "W25Q32", 4 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },
	{ FLASH_ID(0xefu, 0x4017u), "W25Q64", 8 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },
	{ FLASH_ID(0xefu, 0x4018u), "W25Q128", 16 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },
	{ FLASH_ID(0xefu, 0x4019u), "W25Q256JVEIQ", 32 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },
	{ FLASH_ID(0xefu, 0x6019u), "W25Q256JW-Q", 32 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },
	{ FLASH_ID(0xefu, 0x8019u), "W25Q256JW-M", 32 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },

	/* ISSI */
	{ FLASH_ID(0x9du, 0x7016u), "IS25WP032", 4 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },
	{ FLASH_ID(0x9du, 0x7017u), "IS25WP064", 8 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },
	{ FLASH_ID(0x9du, 0x7018u), "IS25WP128", 16 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, NULL },

	/* Micron */
	{ FLASH_ID(0x20u, 0xba19u), "MT25QL256", 32 * 1024 * 1024, 0x100, 0x1000, LUTSZ_MICRON, lutMicron },
	{ FLASH_ID(0x20u, 0xba20u), "MT25QL512", 64 * 1024 * 1024, 0x100, 0x1000, LUTSZ_MICRON, lutMicron },
	{ FLASH_ID(0x20u, 0xba21u), "MT25QL01G", 128 * 1024 * 1024, 0x100, 0x1000, LUTSZ_MICRON, lutMicron },
	{ FLASH_ID(0x20u, 0xba22u), "MT25QL02G", 256 * 1024 * 1024, 0x100, 0x1000, LUTSZ_MICRON, lutMicron },

	/* Macronix (MXIX) */
	{ FLASH_ID(0xc2u, 0x2016u), "MX25L3233", 4 * 1024 * 1024, 0x100, 0x1000, LUTSZ_GENERIC, lutGeneric, nor_mxQuadEnable },
};


static int nor_readID(qspi_t *qspi, u8 port, u32 *retValue, time_t timeout)
{
	struct xferOp xfer;
	u32 lut[4] = { LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_RDID, lutCmdREAD_SDR, lutPad1, 0x04), 0, 0, 0 };

	xfer.op = xfer_opRead;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(qspi_readID);
	xfer.data.read.ptr = retValue;
	xfer.data.read.sz = 3;

	qspi_lutUpdate(qspi, qspi_readID * 4, lut, sizeof(lut) / sizeof(lut[0]));

	return qspi_xferExec(qspi, &xfer);
}


__attribute__((section(".noxip"))) int nor_readStatus(qspi_t *qspi, u8 port, u8 *statusByte, time_t timeout)
{
	struct xferOp xfer;

	xfer.op = xfer_opRead;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(qspi_readStatus);
	xfer.data.read.ptr = statusByte;
	xfer.data.read.sz = 1;

	return qspi_xferExec(qspi, &xfer);
}


__attribute__((section(".noxip"))) int nor_waitBusy(qspi_t *qspi, u8 port, time_t timeout)
{
	int res;
	u8 status = 0;

	do {
		res = nor_readStatus(qspi, port, &status, timeout);
		if (res < EOK) {
			return res;
		}
	} while ((status & 1) != 0);

	return EOK;
}


__attribute__((section(".noxip"))) int nor_writeEnable(qspi_t *qspi, u8 port, int enable, time_t timeout)
{
	struct xferOp xfer;
	int res, seqCode;
	u8 status;

	res = nor_waitBusy(qspi, port, timeout);
	if (res < EOK) {
		return res;
	}

	seqCode = ((enable == 0) ? qspi_writeDisable : qspi_writeEnable);

	xfer.op = xfer_opCommand;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(seqCode);

	res = qspi_xferExec(qspi, &xfer);
	if (res < EOK) {
		return res;
	}

	res = nor_readStatus(qspi, port, &status, timeout);
	if (res < EOK) {
		return res;
	}

	if (((status >> 1) & 1u) == (enable == 0)) {
		return -EACCES;
	}

	return EOK;
}


__attribute__((section(".noxip"))) int nor_eraseSector(qspi_t *qspi, u8 port, addr_t addr, time_t timeout)
{
	struct xferOp xfer;

	int res = nor_writeEnable(qspi, port, 1, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opCommand;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = addr;
	xfer.seqIdx = LUT_SEQIDX(qspi_eraseSector);

	res = qspi_xferExec(qspi, &xfer);
	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(qspi, port, timeout);
}


__attribute__((section(".noxip"))) int nor_eraseChip(qspi_t *qspi, u8 port, time_t timeout)
{
	struct xferOp xfer;

	int res = nor_writeEnable(qspi, port, 1, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opCommand;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = 0;
	xfer.seqIdx = LUT_SEQIDX(qspi_eraseChip);

	res = qspi_xferExec(qspi, &xfer);
	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(qspi, port, timeout);
}


__attribute__((section(".noxip"))) int nor_pageProgram(qspi_t *qspi, u8 port, addr_t dstAddr, const void *src, size_t pageSz, time_t timeout)
{
	struct xferOp xfer;

	int res = nor_writeEnable(qspi, port, 1, timeout);
	if (res < EOK) {
		return res;
	}

	xfer.op = xfer_opWrite;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = dstAddr;
	xfer.seqIdx = LUT_SEQIDX(qspi_programQPP);
	xfer.data.write.ptr = src;
	xfer.data.write.sz = pageSz;

	res = qspi_xferExec(qspi, &xfer);

	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(qspi, port, timeout);
}


ssize_t nor_readData(qspi_t *qspi, u8 port, addr_t addr, void *data, size_t size, time_t timeout)
{
	struct xferOp xfer;

	xfer.op = xfer_opRead;
	xfer.port = port;
	xfer.timeout = timeout;
	xfer.addr = addr;
	xfer.seqIdx = LUT_SEQIDX(qspi_readData);
	xfer.data.read.ptr = data;
	xfer.data.read.sz = size;

	return qspi_xferExec(qspi, &xfer);
}


int nor_probe(qspi_t *qspi, u8 port, const struct nor_info **pInfo, const char **pVendor)
{
	int res;
	unsigned i;
	u32 jedecId = 0;

	res = nor_readID(qspi, port, &jedecId, 1000);
	if (res < EOK) {
		return res;
	}
	log_info("\ndev/flash/nor: Probing flash id 0x%08x", jedecId);

	res = -ENXIO;
	for (i = 0; i < (sizeof(flashInfo) / sizeof(flashInfo[0])); ++i) {
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
