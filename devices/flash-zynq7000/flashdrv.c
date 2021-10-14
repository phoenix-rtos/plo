/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Zynq-7000 nor flash driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "qspi.h"
#include "flashcfg.h"

#include <lib/lib.h>
#include <devices/devs.h>


#define TIMEOUT_CMD_MS 0x05
#define MAX_SIZE_CMD   0x20


struct {
	/* Buffers for command transactions */
	u8 cmdRx[MAX_SIZE_CMD];
	u8 cmdTx[MAX_SIZE_CMD];

	/* Data caching in sector's buffer */
	u8 *buff;
	u32 buffPos;
	u32 regID;
	u32 sectID;

	flash_info_t info;
} fdrv_common;


/* Auxiliary flash commands */

static int flashdrv_cfiRead(flash_cfi_t *cfi)
{
	ssize_t res;
	size_t cmdSz, dataSz;
	/* RDID instruction is a generic for each flash memory */
	const flash_cmd_t cmds = { .opCode = FLASH_CMD_RDID, .size = 1, .dummyCyc = 24 };

	/* Cmd size is rounded to 4 bytes, it includes dummy cycles [b]. */
	cmdSz = (cmds.size + cmds.dummyCyc / 8 + 0x3) & ~0x3;
	/* Size of the data which is received during cmd transfer */
	dataSz = cmds.dummyCyc / 8;

	hal_memset(fdrv_common.cmdTx, 0, cmdSz);
	fdrv_common.cmdTx[0] = cmds.opCode;

	qspi_start();
	if ((res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmdSz, TIMEOUT_CMD_MS)) < 0) {
		qspi_stop();
		return res;
	}

	/* Copy data received after dummy bytes */
	hal_memcpy(cfi, (fdrv_common.cmdRx + cmds.size), dataSz);
	res = qspi_polledTransfer(NULL, (u8 *)cfi + dataSz, sizeof(flash_cfi_t) - dataSz, 5 * TIMEOUT_CMD_MS);
	qspi_stop();

	return res;
}


static int flashdrv_statusRegGet(unsigned int *val)
{
	ssize_t res;
	size_t cmdSz;
	const flash_cmd_t *cmd = &fdrv_common.info.cmds[flash_cmd_rdsr1];

	*val = 0;

	cmdSz = cmd->size + cmd->dummyCyc / 8;
	hal_memset(fdrv_common.cmdTx, 0, cmdSz);

	fdrv_common.cmdTx[0] = cmd->opCode;

	qspi_start();
	res = qspi_polledTransfer(fdrv_common.cmdTx, (u8 *)val, cmdSz, TIMEOUT_CMD_MS);
	qspi_stop();

	return res;
}


static int flashdrv_wipCheck(time_t timeout)
{
	int res;
	unsigned int st;
	time_t start = hal_timerGet();

	do {
		if ((res = flashdrv_statusRegGet(&st)) < 0)
			return res;

		if ((hal_timerGet() - start) >= timeout)
			return -ETIME;
	} while ((st >> 8) & 0x1);

	return EOK;
}


static int flashdrv_welSet(unsigned int cmdID)
{
	ssize_t res;
	const flash_cmd_t *cmd;

	if (cmdID != flash_cmd_wrdi && cmdID != flash_cmd_wren)
		return -EINVAL;

	cmd = &fdrv_common.info.cmds[cmdID];

	hal_memset(fdrv_common.cmdTx, 0, cmd->size);
	fdrv_common.cmdTx[0] = cmd->opCode;

	qspi_start();
	res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmd->size, TIMEOUT_CMD_MS);
	qspi_stop();

	return res;
}


static int flashdrv_regionFind(addr_t offs)
{
	int i;
	size_t sz;
	addr_t start = 0;
	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	for (i = 0; i < cfi->regsCount; ++i) {
		sz = CFI_SIZE_REGION(cfi->regs[i].size, cfi->regs[i].count);
		if (offs >= start && offs < sz)
			return i;

		start = sz;
	}

	return -EINVAL;
}


static int flashdrv_sectorErase(addr_t offs)
{
	int i;
	ssize_t res;
	time_t timeout;
	const flash_cmd_t *cmd;
	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	if (offs > CFI_SIZE_FLASH(cfi->chipSize))
		return -EINVAL;

	if ((i = flashdrv_regionFind(offs)) < 0)
		return i;

	switch (CFI_SIZE_SECTION(cfi->regs[i].size)) {
		case 0x1000:
			cmd = &fdrv_common.info.cmds[flash_cmd_p4e];
			break;
		case 0x10000:
			cmd = &fdrv_common.info.cmds[flash_cmd_p64e];
			break;
		default:
			return -EINVAL;
	}

	if ((res = flashdrv_welSet(flash_cmd_wren)) < 0)
		return res;

	hal_memset(fdrv_common.cmdTx, 0, cmd->size);
	fdrv_common.cmdTx[0] = cmd->opCode;
	fdrv_common.cmdTx[1] = (offs >> 16) & 0xff;
	fdrv_common.cmdTx[2] = (offs >> 8) & 0xff;
	fdrv_common.cmdTx[3] = offs & 0xff;

	qspi_start();
	if ((res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmd->size, TIMEOUT_CMD_MS)) < 0) {
		qspi_stop();
		return res;
	}
	qspi_stop();

	timeout = CFI_TIMEOUT_MAX_ERASE(cfi->timeoutTypical.sectorErase, cfi->timeoutMax.sectorErase) + TIMEOUT_CMD_MS;

	return flashdrv_wipCheck(timeout);
}


static ssize_t flashdrv_pageProgram(addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	time_t timeout;
	const flash_cfi_t *cfi = &fdrv_common.info.cfi;
	const flash_cmd_t *cmd = &fdrv_common.info.cmds[flash_cmd_pp];

	if (len == 0)
		return 0;

	if (buff == NULL || len % CFI_SIZE_PAGE(cfi->pageSize))
		return -EINVAL;

	if ((res = flashdrv_welSet(flash_cmd_wren)) < 0)
		return res;

	hal_memset(fdrv_common.cmdTx, 0, cmd->size);
	fdrv_common.cmdTx[0] = cmd->opCode;
	fdrv_common.cmdTx[1] = (offs >> 16) & 0xff;
	fdrv_common.cmdTx[2] = (offs >> 8) & 0xff;
	fdrv_common.cmdTx[3] = offs & 0xff;

	qspi_start();
	if ((res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmd->size, TIMEOUT_CMD_MS)) < 0) {
		qspi_stop();
		return res;
	}

	/* Transmit data to write */
	if ((res = qspi_polledTransfer(buff, NULL, len, TIMEOUT_CMD_MS)) < 0) {
		qspi_stop();
		return res;
	}
	qspi_stop();

	len = res;

	timeout = CFI_TIMEOUT_MAX_PROGRAM(cfi->timeoutTypical.pageWrite, cfi->timeoutMax.pageWrite) + TIMEOUT_CMD_MS;
	if ((res = flashdrv_wipCheck(timeout)) < 0)
		return res;

	return len;
}


/* Device interface */

static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	ssize_t res;
	size_t cmdSz, dataSz;
	const flash_cmd_t *cmd = &fdrv_common.info.cmds[flash_cmd_qior];

	if (len == 0)
		return 0;

	if (buff == NULL)
		return -EINVAL;

	hal_memset(fdrv_common.cmdTx, 0, cmd->size);
	fdrv_common.cmdTx[0] = cmd->opCode;
	fdrv_common.cmdTx[1] = (offs >> 16) & 0xff;
	fdrv_common.cmdTx[2] = (offs >> 8) & 0xff;
	fdrv_common.cmdTx[3] = offs & 0xff;

	/* Cmd size is rounded to 4 bytes, it includes dummy cycles [b]. */
	cmdSz = (cmd->size + cmd->dummyCyc / 8 + 0x3) & ~0x3;

	/* Size of the data which is received during cmd transfer */
	dataSz = cmdSz - cmd->size - cmd->dummyCyc / 8;

	qspi_start();
	if ((res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmdSz, TIMEOUT_CMD_MS)) < 0) {
		qspi_stop();
		return res;
	}
	/* Copy data received after dummy bytes */
	hal_memcpy(buff, (fdrv_common.cmdRx + cmd->size + cmd->dummyCyc / 8), dataSz);

	res = qspi_polledTransfer(NULL, (u8 *)buff + dataSz, len - dataSz, timeout);
	qspi_stop();

	return (res < 0) ? res : (res + dataSz);
}


static int flashdrv_sync(unsigned int minor)
{
	u8 *src;
	addr_t dst;
	ssize_t res;
	unsigned int i, pgNb;

	size_t pageSz, sectSz, regStart = 0;
	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	if (fdrv_common.buffPos == 0)
		return EOK;

	if (fdrv_common.regID)
		regStart = CFI_SIZE_REGION(cfi->regs[fdrv_common.regID - 1].size, cfi->regs[fdrv_common.regID - 1].count);

	sectSz = CFI_SIZE_SECTION(cfi->regs[fdrv_common.regID].size);
	pageSz = CFI_SIZE_PAGE(cfi->pageSize);
	pgNb = sectSz / pageSz;

	for (i = 0; i < pgNb; ++i) {
		dst = regStart + fdrv_common.sectID * sectSz + i * pageSz;
		src = fdrv_common.buff + i * pageSz;

		if ((res = flashdrv_pageProgram(dst, src, pageSz)) < 0)
			return res;
	}

	fdrv_common.regID = -1;
	fdrv_common.sectID = -1;
	fdrv_common.buffPos = 0;

	return EOK;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	time_t timeout;
	size_t chunkSz, freeSz, saveSz = 0;
	static const u32 timeoutFactor = 0x100;

	int regID, sectID;
	size_t sectSz, regStart;

	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	if (len == 0)
		return 0;

	if (buff == NULL || (offs + len) > CFI_SIZE_FLASH(cfi->chipSize))
		return -EINVAL;

	while (saveSz < len) {
		offs += saveSz;

		if ((regID = flashdrv_regionFind(offs)) < 0)
			return -EINVAL;

		regStart = regID ? CFI_SIZE_REGION(cfi->regs[regID - 1].size, cfi->regs[regID - 1].count) : 0;
		sectSz = CFI_SIZE_SECTION(cfi->regs[regID].size);
		sectID = (offs - regStart) / sectSz;

		if (regID != fdrv_common.regID || sectID != fdrv_common.sectID) {
			if ((res = flashdrv_sync(minor)) < 0)
				return res;

			/* Read operation timeout depends on sector size. Factor value selected empirically. */
			timeout = (sectSz * TIMEOUT_CMD_MS) / timeoutFactor;
			if ((res = flashdrv_read(minor, regStart + (sectSz * sectID), fdrv_common.buff, sectSz, timeout)) < 0)
				return res;

			if ((res = flashdrv_sectorErase(regStart + (sectSz * sectID))) < 0)
				return res;

			fdrv_common.regID = regID;
			fdrv_common.sectID = sectID;
			fdrv_common.buffPos = offs - regStart - (sectID * sectSz);
		}

		freeSz = sectSz - fdrv_common.buffPos;
		chunkSz = (freeSz > (len - saveSz)) ? (len - saveSz) : freeSz;
		hal_memcpy(fdrv_common.buff + fdrv_common.buffPos, (const u8 *)buff + saveSz, chunkSz);

		saveSz += chunkSz;
		fdrv_common.buffPos += chunkSz;

		if (fdrv_common.buffPos < sectSz)
			continue;

		if ((res = flashdrv_sync(minor)) < 0)
			return res;
	}

	return saveSz;
}


static int flashdrv_done(unsigned int minor)
{
	int res;

	if ((res = flashdrv_sync(minor)) < 0)
		return res;

	if ((res = qspi_deinit()) < 0)
		return res;

	return EOK;
}


/* XIP is only supported during booting by BootRom */
static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	/* flash is not mappable to any region in I/O mode */
	return dev_isNotMappable;
}


static int flashdrv_init(unsigned int minor)
{
	int i, res;
	flash_info_t *info = &fdrv_common.info;

	fdrv_common.regID = -1;
	fdrv_common.sectID = -1;
	fdrv_common.buffPos = 0;

	/* FIXME: temp solution to allocate buffer in high OCRAM (64KB) */
	fdrv_common.buff = (void *)ADDR_OCRAM_HIGH;

	if ((res = qspi_init()) < 0)
		return res;

	if ((res = flashdrv_cfiRead(&info->cfi)) < 0)
		return res;

	if ((res = flashcfg_infoResolve(info)))
		return res;

	for (i = 0; i < info->cfi.regsCount; ++i) {
		if (CFI_SIZE_SECTION(info->cfi.regs[i].size) > SIZE_OCRAM_HIGH)
			return -EINVAL;
	}

	lib_printf("\ndev/flash: Initializing flash(%d.%d)", DEV_STORAGE, minor);

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_handler_t h = {
		.init = flashdrv_init,
		.done = flashdrv_done,
		.read = flashdrv_read,
		.write = flashdrv_write,
		.sync = flashdrv_sync,
		.map = flashdrv_map,
	};

	devs_register(DEV_STORAGE, 1, &h);
}
