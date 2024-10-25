/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR716 flash driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/log.h>
#include <lib/errno.h>

#include <devices/flash-spimctrl/spimctrl.h>


#define FLASH_ID(vid, pid) ((((vid) & 0xffu) << 16) | ((pid) & 0xff00u) | ((pid) & 0xffu))

#define VID_MACRONIX 0xc2u
#define VID_SPANSION 0x01u

#define FLASH_CMD_RDID 0x9fu

/* Status register */

#define FLASH_SR_WIP 0x01u /* Write in progress */
#define FLASH_SR_WEL 0x02u /* Write enable latch */


/* clang-format off */
enum { write_disable = 0, write_enable };


static const struct nor_cmds nor_macronixCmds = {
	.rdsr = 0x05u, .wren = 0x06u, .wrdi = 0x04u, .rdear = 0xc8u,
	.wrear = 0xc5u, .ce = 0x60u, .se = 0x20u, .pp = 0x02u, .read = 0x03u
};

static const struct nor_cmds nor_spansionCmds = {
	.rdsr = 0x05u, .wren = 0x06u, .wrdi = 0x04u, .rdear = 0x16u,
	.wrear = 0x17u, .ce = 0x60u, .se = 0xd8u, .pp = 0x02u, .read = 0x03u
};

/* clang-format on */

static const struct nor_info flashInfo[] = {
	/* Macronix (MXIX) */
	{
		.jedecId = FLASH_ID(VID_MACRONIX, 0x2019u),
		.name = "MX25L25635F",
		.totalSz = 32 * 1024 * 1024,
		.pageSz = 0x100,
		.sectorSz = 0x1000,
		.tPP = 2,
		.tSE = 120,
		.tCE = 150 * 1000,
		.regions = {
			{ 8192, 0x1000 },
			{ 0, 0 },
		},
	},
	/* Spansion */
	{
		.jedecId = FLASH_ID(VID_SPANSION, 0x2018u),
		.name = "S25FL128S",
		.totalSz = 16 * 1024 * 1024,
		.pageSz = 0x100,
		.sectorSz = 0x10000,
		.tPP = 1,
		.tSE = 650,
		.tCE = 165 * 1000,
		.regions = {
			{ 32, 0x1000 },
			{ 254, 0x10000 },
			{ 0, 0 },
		},
	}
};


static int nor_readId(spimctrl_t *spimctrl, u32 *id)
{
	struct xferOp xfer;
	const u8 cmd = FLASH_CMD_RDID;
	u8 data[3];
	int res;

	xfer.type = xfer_opRead;
	xfer.cmd = &cmd;
	xfer.cmdLen = 1;
	xfer.rxData = data;
	xfer.dataLen = 3;

	res = spimctrl_xfer(spimctrl, &xfer);

	*id = (data[0] << 16) | (data[1] << 8) | data[2];

	return res;
}


static int nor_readStatus(spimctrl_t *spimctrl, u8 *status)
{
	struct xferOp xfer;
	const u8 cmd = spimctrl->dev.cmds->rdsr;

	xfer.type = xfer_opRead;
	xfer.cmd = &cmd;
	xfer.cmdLen = 1;
	xfer.rxData = status;
	xfer.dataLen = 1;

	return spimctrl_xfer(spimctrl, &xfer);
}


static int nor_waitBusy(spimctrl_t *spimctrl, time_t timeout)
{
	int res;
	u8 status = 0;
	const time_t end = hal_timerGet() + timeout;

	do {
		res = nor_readStatus(spimctrl, &status);
		if (res < 0) {
			return res;
		}

		if ((timeout > 0) && (hal_timerGet() > end)) {
			return -ETIME;
		}
	} while ((status & FLASH_SR_WIP) != 0);

	return EOK;
}


static int nor_writeEnable(spimctrl_t *spimctrl, int enable)
{
	int res;
	struct xferOp xfer;
	u8 status = 0;
	const u8 cmd = (enable == 1) ? spimctrl->dev.cmds->wren : spimctrl->dev.cmds->wrdi;

	res = nor_waitBusy(spimctrl, 0);
	if (res < 0) {
		return res;
	}

	xfer.type = xfer_opWrite;
	xfer.cmd = &cmd;
	xfer.cmdLen = 1;
	xfer.txData = NULL;
	xfer.dataLen = 0;

	res = spimctrl_xfer(spimctrl, &xfer);
	if (res < EOK) {
		return res;
	}

	res = nor_readStatus(spimctrl, &status);
	if (res < EOK) {
		return res;
	}

	status = (status & FLASH_SR_WEL) ? 1 : 0;

	if (status != enable) {
		return -EIO;
	}

	return EOK;
}


static int nor_readEAR(spimctrl_t *spimctrl, u8 *status)
{
	struct xferOp xfer;
	const u8 cmd = spimctrl->dev.cmds->rdear;

	xfer.type = xfer_opRead;
	xfer.cmd = &cmd;
	xfer.cmdLen = 1;
	xfer.rxData = status;
	xfer.dataLen = 1;

	return spimctrl_xfer(spimctrl, &xfer);
}


static int nor_writeEAR(spimctrl_t *spimctrl, u8 value)
{
	int res;
	struct xferOp xfer;
	const u8 cmd = spimctrl->dev.cmds->wrear;

	nor_writeEnable(spimctrl, write_enable);

	xfer.type = xfer_opWrite;
	xfer.cmd = &cmd;
	xfer.cmdLen = 1;
	xfer.txData = &value;
	xfer.dataLen = 1;

	res = spimctrl_xfer(spimctrl, &xfer);
	if (res < EOK) {
		return res;
	}

	res = nor_readEAR(spimctrl, &spimctrl->dev.ear);
	if (res < EOK) {
		return res;
	}

	if (spimctrl->dev.ear != value) {
		return -EIO;
	}

	return EOK;
}


static int nor_validateEar(spimctrl_t *spimctrl, addr_t addr)
{
	int res = EOK;
	const u8 desiredEar = (addr >> 24) & 0xffu;

	if (desiredEar != spimctrl->dev.ear) {
		res = nor_writeEAR(spimctrl, desiredEar);
	}
	return res;
}


int nor_eraseChip(spimctrl_t *spimctrl, time_t timeout)
{
	int res;
	struct xferOp xfer;
	const u8 cmd = spimctrl->dev.cmds->ce;

	res = nor_writeEnable(spimctrl, write_enable);
	if (res < EOK) {
		return res;
	}

	xfer.type = xfer_opWrite;
	xfer.cmd = &cmd;
	xfer.cmdLen = 1;
	xfer.txData = NULL;
	xfer.dataLen = 0;

	res = spimctrl_xfer(spimctrl, &xfer);
	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(spimctrl, timeout);
}


int nor_eraseSector(spimctrl_t *spimctrl, addr_t addr, time_t timeout)
{
	int res = EOK;
	struct xferOp xfer;
	u8 cmd[4] = { spimctrl->dev.cmds->se, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff };
	size_t regionEraseSz = 0, regionEnd = 0;
	size_t i;

	for (i = 0; i < NOR_REGIONS_MAX; i++) {
		if (spimctrl->dev.info->regions[i].sectorCnt == 0) {
			break;
		}

		regionEnd += (spimctrl->dev.info->regions[i].sectorCnt * spimctrl->dev.info->regions[i].sectorSz);
		if (addr < regionEnd) {
			regionEraseSz = spimctrl->dev.info->regions[i].sectorSz;
			break;
		}
	}

	if (regionEraseSz == 0) {
		return -EINVAL;
	}

	for (i = 0; i < spimctrl->dev.info->sectorSz / regionEraseSz; i++) {
		res = nor_validateEar(spimctrl, addr);
		if (res < EOK) {
			return res;
		}

		res = nor_writeEnable(spimctrl, write_enable);
		if (res < EOK) {
			return res;
		}

		xfer.type = xfer_opWrite;
		xfer.cmd = cmd;
		xfer.cmdLen = 4;
		xfer.txData = NULL;
		xfer.dataLen = 0;

		res = spimctrl_xfer(spimctrl, &xfer);
		if (res < EOK) {
			return res;
		}

		res = nor_waitBusy(spimctrl, timeout);

		if (res < EOK) {
			return res;
		}

		addr += regionEraseSz;
		cmd[1] = (addr >> 16) & 0xff;
		cmd[2] = (addr >> 8) & 0xff;
		cmd[3] = addr & 0xff;
	}

	return res;
}


int nor_pageProgram(spimctrl_t *spimctrl, addr_t addr, const void *src, size_t len, time_t timeout)
{
	struct xferOp xfer;
	const u8 cmd[4] = { spimctrl->dev.cmds->pp, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff };

	int res = nor_validateEar(spimctrl, addr);
	if (res < EOK) {
		return res;
	}

	res = nor_writeEnable(spimctrl, write_enable);
	if (res < EOK) {
		return res;
	}

	xfer.type = xfer_opWrite;
	xfer.cmd = cmd;
	xfer.cmdLen = 4;
	xfer.txData = src;
	xfer.dataLen = len;

	res = spimctrl_xfer(spimctrl, &xfer);
	if (res < EOK) {
		return res;
	}

	res = nor_waitBusy(spimctrl, timeout);

	return res;
}

static ssize_t nor_readCmd(spimctrl_t *spimctrl, addr_t addr, void *data, size_t size)
{
	struct xferOp xfer;
	const u8 cmd[4] = { spimctrl->dev.cmds->read, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff };

	int res = nor_validateEar(spimctrl, addr);
	if (res < EOK) {
		return res;
	}

	xfer.type = xfer_opRead;
	xfer.cmd = cmd;
	xfer.cmdLen = 4;
	xfer.rxData = data;
	xfer.dataLen = size;

	res = spimctrl_xfer(spimctrl, &xfer);

	return (res < EOK) ? res : (ssize_t)size;
}


static ssize_t nor_readAhb(spimctrl_t *spimctrl, addr_t addr, void *data, size_t size)
{
	int res = nor_validateEar(spimctrl, addr);
	if (res < EOK) {
		return res;
	}

	hal_memcpy(data, (void *)addr + spimctrl->maddr, size);

	return (ssize_t)size;
}


ssize_t nor_readData(spimctrl_t *spimctrl, addr_t addr, void *data, size_t size)
{
	if (((addr & 0xff000000) == 0) && (((addr + size) & 0xff000000) != 0)) {
		/* If we'd have to change EAR register during read,
		 * read data through command (can be read without EAR change)
		 */
		return nor_readCmd(spimctrl, addr, data, size);
	}
	else {
		/* Direct copy */
		return nor_readAhb(spimctrl, addr, data, size);
	}
}


static int nor_probe(spimctrl_t *spimctrl, const char **pVendor)
{
	int res;
	size_t i;
	u32 jedecId = 0;

	res = nor_readId(spimctrl, &jedecId);
	if (res < EOK) {
		return res;
	}
	log_info("\ndev/flash/nor: Probing flash id 0x%08x", jedecId);

	res = -ENXIO;
	for (i = 0; i < sizeof(flashInfo) / sizeof(flashInfo[0]); ++i) {
		if (flashInfo[i].jedecId == jedecId) {
			spimctrl->dev.info = &flashInfo[i];
			res = EOK;
			break;
		}
	}

	if (res != EOK) {
		log_error("\ndev/flash/nor: Unsupported flash id 0x%08x", jedecId);
		return res;
	}

	switch (jedecId >> 16) {
		case VID_MACRONIX:
			spimctrl->dev.cmds = &nor_macronixCmds;
			*pVendor = "Macronix";
			break;

		case VID_SPANSION:
			spimctrl->dev.cmds = &nor_spansionCmds;
			*pVendor = "Spansion";
			break;

		default:
			/* Should never be reached */
			res = -ENXIO;
			break;
	}

	return res;
}


int nor_deviceInit(spimctrl_t *spimctrl, const char **vendor)
{
	int res = nor_probe(spimctrl, vendor);
	if (res != EOK) {
		log_error("\ndev/flash: Initialization failed");
		return res;
	}

	res = nor_readEAR(spimctrl, &spimctrl->dev.ear);
	if (res < EOK) {
		return res;
	}

	spimctrl->dev.sectorBufAddr = (addr_t)-1;
	spimctrl->dev.sectorBufDirty = 0;
	hal_memset(spimctrl->dev.sectorBuf, NOR_ERASED_STATE, sizeof(spimctrl->dev.sectorBuf));

	return EOK;
}
