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


// #define FLASH_ID(vid, pid) ((((vid) & 0xffu) << 16) | ((pid) & 0xff00u) | ((pid) & 0xffu))
// #define SPIM_LITTLE_ENDIAN 1
#if defined(SPIM_LITTLE_ENDIAN) && (SPIM_LITTLE_ENDIAN != 0)
#define FLASH_ID(vid, pid) ((((pid) & 0xffu) << 16) | ((pid) & 0xff00u) | ((vid) & 0xffu)) /* if little endian*/
#else
#define FLASH_ID(vid, pid) ((((vid) & 0xffu) << 16) | ((pid) & 0xff00u) | ((pid) & 0xffu))
#endif /* SPIM_LITTLE_ENDIAN */

#define VID_MACRONIX 0xc2u
#define VID_SPANSION 0x01u
#define VID_MICRON   0x20u

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

static const struct nor_cmds nor_micronCmds = {
	.rdsr = 0x05u, .wren = 0x06u, .wrdi = 0x04u, .rdear = 0xc8u,
	.wrear = 0xc5u, .ce = 0xc4u, .se = 0xdcu, .pp = 0x12u, .read = 0x13u
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
	},
	/* Micron */
	{
		.jedecId = FLASH_ID(VID_MICRON, 0xBB21u),
		.name = "MT25QU01GB",
		.totalSz = 128 * 1024 * 1024,
		.pageSz = 0x100,
		.sectorSz = 0x10000,
		.tPP = 2,
		.tSE = 1000,
		.tCE = 920 * 1000,
		.regions = { { 0, 0 } },
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

#if defined(SPIM_LITTLE_ENDIAN) && (SPIM_LITTLE_ENDIAN != 0)
	*id = ((u32)data[2] << 16) | ((u32)data[1] << 8) | data[0];
#else
	*id = (data[0] << 16) | (data[1] << 8) | data[2];
#endif /* SPIM_LITTLE_ENDIAN */

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


int nor_eraseDie(spimctrl_t *spimctrl, time_t timeout, u8 selDie)
{
	int res;
	u8 cmd[5];
	struct xferOp xfer;

	size_t flashSize;
	flashSize = spimctrl->dev.info->totalSz;

	addr_t addr = 0x00000000u;
	addr_t addr0 = 0x00000000u;
	addr_t addr1 = flashSize / 2;

	if (selDie == 0) {
		log_info("Erasing first die \n");
		addr = addr0;
	}
	else if (selDie == 1) {
		log_info("Erasing second die \n");
		addr = addr1;
	}

	cmd[0] = spimctrl->dev.cmds->ce;

	if ((!spimctrl->extendedAddress)) {
		cmd[1] = (addr >> 16) & 0xff;
		cmd[2] = (addr >> 8) & 0xff;
		cmd[3] = addr & 0xff;

		res = nor_validateEar(spimctrl, addr);
		if (res < EOK) {
			return res;
		}

		xfer.cmd = cmd;
		xfer.cmdLen = 4;
	}
	else {
		cmd[1] = (addr >> 24) & 0xff;
		cmd[2] = (addr >> 16) & 0xff;
		cmd[3] = (addr >> 8) & 0xff;
		cmd[4] = addr & 0xff;

		xfer.cmd = cmd;
		xfer.cmdLen = 5;
	}

	res = nor_writeEnable(spimctrl, write_enable);
	if (res < EOK) {
		return res;
	}

	xfer.type = xfer_opWrite;
	xfer.txData = NULL;
	xfer.dataLen = 0;

	res = spimctrl_xfer(spimctrl, &xfer);
	if (res < EOK) {
		return res;
	}

	return nor_waitBusy(spimctrl, timeout);
}


int nor_eraseChip(spimctrl_t *spimctrl, time_t timeout)
{
	int res;
	struct xferOp xfer;

	if (spimctrl->dev.info->jedecId == 0x21bb20) {
		log_info("Erasing stacked device \n");
		res = nor_eraseDie(spimctrl, timeout, 0);
		if (res < EOK) {
			return res;
		}
		res = nor_eraseDie(spimctrl, timeout, 1);
	}
	else {
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

		res = nor_waitBusy(spimctrl, timeout);
	}

	return res;
}


static inline u8 nor_isHybrid(const struct nor_info *info)
{
	return info->regions[0].sectorCnt != 0;
}


int nor_eraseSector(spimctrl_t *spimctrl, addr_t addr, time_t timeout)
{
	int res = EOK;
	struct xferOp xfer;
	size_t regionEraseSz = 0;
	size_t numSectors, i;
	u8 cmd[5];
	cmd[0] = spimctrl->dev.cmds->se;

	if (!nor_isHybrid(spimctrl->dev.info)) {
		regionEraseSz = spimctrl->dev.info->sectorSz;
	}
	else {
		size_t regionEnd = 0;

		for (i = 0; i < NOR_REGIONS_MAX; i++) {
			if (spimctrl->dev.info->regions[i].sectorCnt == 0) {
				break;
			}

			regionEnd += (spimctrl->dev.info->regions[i].sectorCnt *
					spimctrl->dev.info->regions[i].sectorSz);

			if (addr < regionEnd) {
				regionEraseSz = spimctrl->dev.info->regions[i].sectorSz;
				break;
			}
		}
	}

	if (regionEraseSz == 0) {
		return -EINVAL;
	}

	numSectors = spimctrl->dev.info->sectorSz / regionEraseSz;
	for (i = 0; i < numSectors; i++) {

		if ((!spimctrl->extendedAddress)) {
			res = nor_validateEar(spimctrl, addr);
			if (res < EOK) {
				return res;
			}

			cmd[1] = (addr >> 16) & 0xff;
			cmd[2] = (addr >> 8) & 0xff;
			cmd[3] = addr & 0xff;
			xfer.cmdLen = 4;
		}
		else {
			cmd[1] = (addr >> 24) & 0xff;
			cmd[2] = (addr >> 16) & 0xff;
			cmd[3] = (addr >> 8) & 0xff;
			cmd[4] = addr & 0xff;
			xfer.cmdLen = 5;
		}

		res = nor_writeEnable(spimctrl, write_enable);
		if (res < EOK) {
			return res;
		}

		xfer.type = xfer_opWrite;
		xfer.cmd = cmd;
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
	}

	return res;
}


int nor_pageProgram(spimctrl_t *spimctrl, addr_t addr, const void *src, size_t len, time_t timeout)
{
	log_error("\ndev/flash/nor: page program");
	struct xferOp xfer;
	u8 cmd[5];
	cmd[0] = spimctrl->dev.cmds->pp;
	int res;

	if ((!spimctrl->extendedAddress)) {
		res = nor_validateEar(spimctrl, addr);
		if (res < EOK) {
			return res;
		}

		cmd[1] = (addr >> 16) & 0xff;
		cmd[2] = (addr >> 8) & 0xff;
		cmd[3] = addr & 0xff;
		xfer.cmdLen = 4;
	}
	else {
		cmd[1] = (addr >> 24) & 0xff;
		cmd[2] = (addr >> 16) & 0xff;
		cmd[3] = (addr >> 8) & 0xff;
		cmd[4] = addr & 0xff;
		xfer.cmdLen = 5;
	}

	res = nor_writeEnable(spimctrl, write_enable);
	if (res < EOK) {
		return res;
	}

	xfer.type = xfer_opWrite;
	xfer.cmd = cmd;
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
	u8 cmd[5];
	cmd[0] = spimctrl->dev.cmds->read;
	int res;

	if ((!spimctrl->extendedAddress)) {
		res = nor_validateEar(spimctrl, addr);
		if (res < EOK) {
			return res;
		}

		cmd[1] = (addr >> 16) & 0xff;
		cmd[2] = (addr >> 8) & 0xff;
		cmd[3] = addr & 0xff;
		xfer.cmdLen = 4;
	}
	else {
		cmd[1] = (addr >> 24) & 0xff;
		cmd[2] = (addr >> 16) & 0xff;
		cmd[3] = (addr >> 8) & 0xff;
		cmd[4] = addr & 0xff;
		xfer.cmdLen = 5;
	}

	xfer.type = xfer_opRead;
	xfer.cmd = cmd;
	xfer.rxData = data;
	xfer.dataLen = size;

	res = spimctrl_xfer(spimctrl, &xfer);

	return (res < EOK) ? res : (ssize_t)size;
}


static ssize_t nor_readAhb(spimctrl_t *spimctrl, addr_t addr, void *data, size_t size)
{
	if ((!spimctrl->extendedAddress)) {
		int res = nor_validateEar(spimctrl, addr);
		if (res < EOK) {
			return res;
		}
	}

	hal_memcpy(data, (void *)addr + spimctrl->maddr, size);

	return (ssize_t)size;
}


ssize_t nor_readData(spimctrl_t *spimctrl, addr_t addr, void *data, size_t size)
{
	if ((!spimctrl->extendedAddress)) {
		if ((addr & 0xff000000) != ((addr + size - 1) & 0xff000000)) {
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
	else {
		return nor_readCmd(spimctrl, addr, data, size);
	}
}


static int nor_probe(spimctrl_t *spimctrl, const char **pVendor)
{
	int res;
	size_t i;
	u32 jedecId = 0;

	res = nor_readId(spimctrl, &jedecId);
	log_error("\ndev/flash/nor: flash id 0x%08x", jedecId);
	if (res < EOK) {
		return res;
	}
	log_info("\ndev/flash/nor: Probing flash id 0x%08x", jedecId);

	res = -ENXIO;
	for (i = 0; i < sizeof(flashInfo) / sizeof(flashInfo[0]); ++i) {
		if ((flashInfo[i].jedecId & 0x00FFFFFFu) == (jedecId & 0x00FFFFFFu)) {
			spimctrl->dev.info = &flashInfo[i];
			res = EOK;
			break;
		}
	}

	if (res != EOK) {
		log_error("\ndev/flash/nor: Unsupported flash id 0x%08x", jedecId);
		return res;
	}

#if defined(SPIM_LITTLE_ENDIAN) && (SPIM_LITTLE_ENDIAN != 0)
	u32 vid = jedecId & 0xFF;
	log_error("\ndev/flash/nor: flash vid 0x%08x", vid);
#else
	u32 vid = jedecId >> 16;
	log_error("\ndev/flash/nor: flash vid 0x%08x", vid);
#endif

	switch (vid) {
		case VID_MACRONIX:
			spimctrl->dev.cmds = &nor_macronixCmds;
			*pVendor = "Macronix";
			break;

		case VID_SPANSION:
			spimctrl->dev.cmds = &nor_spansionCmds;
			*pVendor = "Spansion";
			break;

		case VID_MICRON:
			spimctrl->dev.cmds = &nor_micronCmds;
			*pVendor = "Micron";
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

	// if ((!spimctrl->extendedAddress)) {
	// 	int res = nor_validateEar(spimctrl, addr);
	// 	if (res < EOK) {
	// 		return res;
	// 	}
	// }

	spimctrl->dev.sectorBufAddr = (addr_t)-1;
	spimctrl->dev.sectorBufDirty = 0;
	hal_memset(spimctrl->dev.sectorBuf, NOR_ERASED_STATE, sizeof(spimctrl->dev.sectorBuf));

	return EOK;
}
