/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Zynq-7000 nor flash driver
 *
 * Copyright 2021-2022 Phoenix Systems
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


static size_t flashdrv_regStart(int id)
{
	int i;
	size_t regStart = 0;
	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	for (i = 0; i < id; ++i) {
		regStart += CFI_SIZE_REGION(cfi->regs[i].size, cfi->regs[i].count);
	}

	return regStart;
}


static int flashdrv_regionFind(addr_t offs, u32 *id)
{
	int i;
	size_t sz;
	addr_t start = 0;
	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	for (i = 0; i < cfi->regsCount; ++i) {
		sz = CFI_SIZE_REGION(cfi->regs[i].size, cfi->regs[i].count);
		if (offs >= start && offs < (start + sz)) {
			*id = i;
			return EOK;
		}

		start = sz;
	}

	return -EINVAL;
}


static inline void flashdrv_serializeTxCmd(u8 *buff, flash_cmd_t cmd, addr_t offs)
{
	hal_memset(buff, 0, cmd.size);
	buff[0] = cmd.opCode;

	if (fdrv_common.info.addrMode == flash_4byteAddr) {
		buff[1] = (offs >> 24) & 0xff;
		buff[2] = (offs >> 16) & 0xff;
		buff[3] = (offs >> 8) & 0xff;
		buff[4] = offs & 0xff;
	}
	else {
		buff[1] = (offs >> 16) & 0xff;
		buff[2] = (offs >> 8) & 0xff;
		buff[3] = offs & 0xff;
	}
}


/* Auxiliary flash commands */

static int flashdrv_cfiRead(flash_cfi_t *cfi)
{
	ssize_t res;
	size_t cmdSz, dataSz;
	flash_cmd_t cmd;

	flashcfg_jedecIDGet(&cmd);

	/* Cmd size is rounded to 4 bytes, it includes dummy cycles [b]. */
	cmdSz = (cmd.size + (cmd.dummyCyc * cmd.dataLines) / 8 + 0x3) & ~0x3;
	/* Size of the data which is received during cmd transfer */
	dataSz = cmdSz - cmd.size;

	hal_memset(fdrv_common.cmdTx, 0, cmdSz);
	fdrv_common.cmdTx[0] = cmd.opCode;

	qspi_start();
	res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmdSz, TIMEOUT_CMD_MS);
	if (res < 0) {
		qspi_stop();
		return res;
	}

	/* Copy data received after dummy bytes */
	hal_memcpy(cfi, (fdrv_common.cmdRx + cmd.size), dataSz);
	res = qspi_polledTransfer(NULL, (u8 *)cfi + dataSz, sizeof(flash_cfi_t) - dataSz, 5 * TIMEOUT_CMD_MS);
	qspi_stop();

	return res;
}


static int flashdrv_statusRegGet(unsigned int *val)
{
	ssize_t res;
	size_t cmdSz;
	const flash_cmd_t cmd = fdrv_common.info.cmds[flash_cmd_rdsr1];

	*val = 0;

	cmdSz = cmd.size + (cmd.dummyCyc * cmd.dataLines) / 8;
	hal_memset(fdrv_common.cmdTx, 0, cmdSz);

	qspi_start();
	fdrv_common.cmdTx[0] = cmd.opCode;
	res = qspi_polledTransfer(fdrv_common.cmdTx, (u8 *)val, cmdSz, TIMEOUT_CMD_MS);
	qspi_stop();

	return res;
}


static int flashdrv_wipCheck(time_t timeout)
{
	int res;
	unsigned int st;
	const time_t stop = hal_timerGet() + timeout;

	do {
		res = flashdrv_statusRegGet(&st);
		if (res < 0) {
			return res;
		}

		if (hal_timerGet() >= stop) {
			return -ETIME;
		}
	} while ((st >> 8) & 0x1);

	return EOK;
}


static int flashdrv_welSet(unsigned int cmdID)
{
	ssize_t res;
	flash_cmd_t cmd;

	if ((cmdID != flash_cmd_wrdi) && (cmdID != flash_cmd_wren)) {
		return -EINVAL;
	}

	cmd = fdrv_common.info.cmds[cmdID];

	hal_memset(fdrv_common.cmdTx, 0, cmd.size);
	fdrv_common.cmdTx[0] = cmd.opCode;

	qspi_start();
	res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmd.size, TIMEOUT_CMD_MS);
	qspi_stop();

	return res;
}


static int flashdrv_chipErase(void)
{
	int res;
	time_t timeout;

	const flash_cfi_t *cfi = &fdrv_common.info.cfi;
	flash_cmd_t cmd = fdrv_common.info.cmds[flash_cmd_be];

	res = flashdrv_welSet(flash_cmd_wren);
	if (res < 0) {
		return res;
	}

	hal_memset(fdrv_common.cmdTx, 0, cmd.size);
	fdrv_common.cmdTx[0] = cmd.opCode;

	qspi_start();
	res = (int)qspi_polledTransfer(fdrv_common.cmdTx, NULL, cmd.size, TIMEOUT_CMD_MS);
	qspi_stop();

	if (res < 0) {
		return res;
	}

	timeout = CFI_TIMEOUT_MAX_ERASE(cfi->timeoutTypical.chipErase, cfi->timeoutMax.chipErase) + TIMEOUT_CMD_MS;

	return flashdrv_wipCheck(timeout);
}


static int flashdrv_sectorErase(addr_t offs, size_t sectSz)
{
	int res;
	time_t timeout;
	flash_cmd_t cmd;
	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	if (offs > CFI_SIZE_FLASH(cfi->chipSize)) {
		return -EINVAL;
	}

	switch (sectSz) {
		case 0x1000:
			cmd = (fdrv_common.info.addrMode == flash_4byteAddr) ? fdrv_common.info.cmds[flash_cmd_4p4e] : fdrv_common.info.cmds[flash_cmd_p4e];
			break;
		case 0x10000:
			cmd = (fdrv_common.info.addrMode == flash_4byteAddr) ? fdrv_common.info.cmds[flash_cmd_4p64e] : fdrv_common.info.cmds[flash_cmd_p64e];
			break;
		default:
			return -EINVAL;
	}

	res = flashdrv_welSet(flash_cmd_wren);
	if (res < 0) {
		return res;
	}

	flashdrv_serializeTxCmd(fdrv_common.cmdTx, cmd, offs);

	qspi_start();
	res = (int)qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmd.size, TIMEOUT_CMD_MS);
	if (res < 0) {
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
	flash_cmd_t cmd = fdrv_common.info.cmds[fdrv_common.info.ppCmd];

	if (len == 0) {
		return 0;
	}

	if ((buff == NULL) || ((len % CFI_SIZE_PAGE(cfi->pageSize)) != 0)) {
		return -EINVAL;
	}

	res = flashdrv_welSet(flash_cmd_wren);
	if (res < 0) {
		return res;
	}

	flashdrv_serializeTxCmd(fdrv_common.cmdTx, cmd, offs);

	qspi_start();
	res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, cmd.size, TIMEOUT_CMD_MS);
	if (res < 0) {
		qspi_stop();
		return res;
	}

	/* Transmit data to write */
	res = qspi_polledTransfer(buff, NULL, len, TIMEOUT_CMD_MS);
	if (res < 0) {
		qspi_stop();
		return res;
	}
	qspi_stop();

	len = res;

	timeout = CFI_TIMEOUT_MAX_PROGRAM(cfi->timeoutTypical.pageWrite, cfi->timeoutMax.pageWrite) + TIMEOUT_CMD_MS;
	res = flashdrv_wipCheck(timeout);
	if (res < 0) {
		return res;
	}

	return len;
}


/* Device interface */

static ssize_t flashdrv_read(unsigned int minor, addr_t offs, void *buff, size_t len, time_t timeout)
{
	ssize_t res;
	size_t cmdSz, dataSz, transferSz, paddedCmdSz, dummySz;
	const flash_cmd_t cmd = fdrv_common.info.cmds[fdrv_common.info.readCmd];

	if (len == 0) {
		return 0;
	}

	if (buff == NULL) {
		return -EINVAL;
	}

	flashdrv_serializeTxCmd(fdrv_common.cmdTx, cmd, offs);

	dummySz = (cmd.dummyCyc * cmd.dataLines) / 8;
	cmdSz = cmd.size + dummySz;

	/* Send 0xff as dummy byte as some flashes require dummy byte that starts with 0xf. */
	hal_memset(fdrv_common.cmdTx + cmdSz, 0xff, dummySz);

	/* Cmd size is rounded to 4 bytes, it includes dummy cycles [b]. */
	paddedCmdSz = (cmdSz + 0x3) & ~0x3;

	/* Size of the data which is received during cmd transfer */
	transferSz = paddedCmdSz - cmdSz;
	dataSz = (transferSz > len) ? len : transferSz;

	qspi_start();
	res = qspi_polledTransfer(fdrv_common.cmdTx, fdrv_common.cmdRx, paddedCmdSz, TIMEOUT_CMD_MS);
	if (res < 0) {
		qspi_stop();
		return res;
	}
	/* Copy data received after dummy bytes */
	hal_memcpy(buff, (fdrv_common.cmdRx + cmdSz), dataSz);

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

	if (fdrv_common.buffPos == 0) {
		return EOK;
	}

	regStart = flashdrv_regStart(fdrv_common.regID);
	sectSz = CFI_SIZE_SECTION(cfi->regs[fdrv_common.regID].size);
	pageSz = CFI_SIZE_PAGE(cfi->pageSize);
	pgNb = sectSz / pageSz;

	for (i = 0; i < pgNb; ++i) {
		dst = regStart + fdrv_common.sectID * sectSz + i * pageSz;
		src = fdrv_common.buff + i * pageSz;

		res = flashdrv_pageProgram(dst, src, pageSz);
		if (res < 0) {
			return res;
		}
	}

	fdrv_common.regID = (u32)-1;
	fdrv_common.sectID = (u32)-1;
	fdrv_common.buffPos = 0;

	return EOK;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const void *buff, size_t len)
{
	ssize_t res;
	time_t timeout;
	size_t chunkSz = 0, freeSz = 0, saveSz = 0;
	static const u32 timeoutFactor = 0x100;

	u32 regID, sectID;
	size_t sectSz, regStart;

	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	if (len == 0) {
		return 0;
	}

	if ((buff == NULL) || ((offs + len) > CFI_SIZE_FLASH(cfi->chipSize))) {
		return -EINVAL;
	}

	while (saveSz < len) {
		offs += chunkSz;

		res = flashdrv_regionFind(offs, &regID);
		if (res < 0) {
			return res;
		}

		regStart = flashdrv_regStart(regID);
		sectSz = CFI_SIZE_SECTION(cfi->regs[regID].size);
		sectID = (offs - regStart) / sectSz;

		if (regID != fdrv_common.regID || sectID != fdrv_common.sectID) {
			res = flashdrv_sync(minor);
			if (res < 0) {
				return res;
			}

			/* Read operation timeout depends on sector size. Factor value selected empirically. */
			timeout = (sectSz * TIMEOUT_CMD_MS) / timeoutFactor;
			res = flashdrv_read(minor, regStart + (sectSz * sectID), fdrv_common.buff, sectSz, timeout);
			if (res < 0) {
				return res;
			}

			res = flashdrv_sectorErase(regStart + (sectSz * sectID), sectSz);
			if (res < 0) {
				return res;
			}

			fdrv_common.regID = regID;
			fdrv_common.sectID = sectID;
			fdrv_common.buffPos = offs - regStart - (sectID * sectSz);
		}

		freeSz = sectSz - fdrv_common.buffPos;
		chunkSz = (freeSz > (len - saveSz)) ? (len - saveSz) : freeSz;
		hal_memcpy(fdrv_common.buff + fdrv_common.buffPos, (const u8 *)buff + saveSz, chunkSz);

		saveSz += chunkSz;
		fdrv_common.buffPos += chunkSz;

		if (fdrv_common.buffPos < sectSz) {
			continue;
		}

		res = flashdrv_sync(minor);
		if (res < 0) {
			return res;
		}
	}

	return saveSz;
}


static ssize_t flashdrv_erase(unsigned int minor, addr_t addr, size_t len, unsigned int flags)
{
	int res;
	size_t sectSz;
	unsigned int id = 0;
	addr_t end, addr_mask;
	const flash_cfi_t *cfi = &fdrv_common.info.cfi;

	if (len == 0) {
		return 0;
	}

	/* Invalidate sync */
	fdrv_common.regID = (u32)-1;
	fdrv_common.sectID = (u32)-1;
	fdrv_common.buffPos = 0;

	/* Chips erase */
	if (len == (size_t)-1) {
		len = CFI_SIZE_FLASH(cfi->chipSize);
		log_info("\nErasing all data from flash device ...");
		res = flashdrv_chipErase();
		if (res < 0) {
			return res;
		}

		return len;
	}


	/* Erase sectors */

	if ((addr + len) > CFI_SIZE_FLASH(cfi->chipSize)) {
		return -EINVAL;
	}


	/* Calculate sector size of the last address */
	end = addr + len;
	res = flashdrv_regionFind(end - 1, &id);
	if (res < 0) {
		return res;
	}

	sectSz = CFI_SIZE_SECTION(cfi->regs[id].size);

	addr_mask = sectSz - 1;
	end = (end + addr_mask) & ~addr_mask;

	/* Calculate sector size of the start address */
	res = flashdrv_regionFind(addr, &id);
	if (res < 0) {
		return res;
	}

	sectSz = CFI_SIZE_SECTION(cfi->regs[id].size);
	addr_mask = sectSz - 1;
	addr &= ~addr_mask;


	log_info("\nErasing sectors from 0x%x to 0x%x ...", addr, end);

	len = 0;
	while (addr < end) {
		res = flashdrv_regionFind(addr, &id);
		if (res < 0) {
			return res;
		}

		sectSz = CFI_SIZE_SECTION(cfi->regs[id].size);

		res = flashdrv_sectorErase(addr, sectSz);
		if (res < 0) {
			return res;
		}
		addr += sectSz;
		len += sectSz;
	}

	return len;
}


static int flashdrv_done(unsigned int minor)
{
	int res;

	res = flashdrv_sync(minor);
	if (res < 0) {
		qspi_deinit();
		return res;
	}

	res = qspi_deinit();
	if (res < 0) {
		return res;
	}

	return EOK;
}


/* XIP is only supported during booting by BootRom */
static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode) {
		return -EINVAL;
	}

	/* flash is not mappable to any region in I/O mode */
	return dev_isNotMappable;
}


static int flashdrv_init(unsigned int minor)
{
	int i, res;
	flash_info_t *info = &fdrv_common.info;

	fdrv_common.regID = (u32)-1;
	fdrv_common.sectID = (u32)-1;
	fdrv_common.buffPos = 0;

	/* FIXME: temp solution to allocate buffer in high OCRAM (64KB) */
	fdrv_common.buff = (void *)ADDR_OCRAM_HIGH;

	res = qspi_init();
	if (res < 0) {
		return res;
	}

	res = flashdrv_cfiRead(&info->cfi);
	if (res < 0) {
		return res;
	}

	res = flashcfg_infoResolve(info);
	if (res < 0) {
		return res;
	}

	for (i = 0; i < info->cfi.regsCount; ++i) {
		if (CFI_SIZE_SECTION(info->cfi.regs[i].size) > SIZE_OCRAM_HIGH) {
			return -EINVAL;
		}
	}

	if (info->init != NULL) {
		res = info->init(info);
		if (res < 0) {
			return res;
		}
	}

	/* Require data RXed due to dummy cycles to be byte aligned. */
	/* This requirement is in place to simplify implementation of read. */
	if ((info->cmds[info->readCmd].dummyCyc == CFI_DUMMY_CYCLES_NOT_SET) ||
		(((info->cmds[info->readCmd].dummyCyc * info->cmds[info->readCmd].dataLines) % 8) != 0)) {
		return -EINVAL;
	}

	lib_printf("\ndev/flash: Configured %s %dMB nor flash(%d.%d)", info->name,
		CFI_SIZE_FLASH(info->cfi.chipSize) >> 20u, DEV_STORAGE, minor);

	return EOK;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_handler_t h = {
		.init = flashdrv_init,
		.done = flashdrv_done,
		.read = flashdrv_read,
		.write = flashdrv_write,
		.erase = flashdrv_erase,
		.sync = flashdrv_sync,
		.map = flashdrv_map,
	};

	devs_register(DEV_STORAGE, 1, &h);
}
