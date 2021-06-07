/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MX RT Flash driver
 *
 * Copyright 2019, 2020 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "lut.h"
#include "romapi.h"
#include "flashdrv.h"
#include "flashcfg.h"

#include <hal/hal.h>
#include <lib/lib.h>
#include <lib/errno.h>
#include <devices/devs.h>


extern int _fcfb;

struct {
	flash_context_t ctx[FLASH_NO];
	u8 buff[0x100];
} flashdrv_common;


/* Flash config commands */

static int flashdrv_getVendorID(flash_context_t *ctx, u32 *manID)
{
	flexspi_xfer_t xfer;

	/* READ JEDEC ID*/
	ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID] = LUT_INSTR(LUT_CMD_CMD, LUT_PAD1, FLASH_SPANSION_CMD_RDID, LUT_CMD_READ, LUT_PAD1, FLASH_SPANSION_CMD_WRDI);  // 0x2404049f
	ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID + 1] = 0;
	ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID + 2] = 0;
	ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID + 3] = 0;
	flexspi_norFlashUpdateLUT(ctx->instance, READ_JEDEC_ID_SEQ_ID, (const u32 *)&ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID], 1);

	xfer.txBuffer = NULL;
	xfer.txSize = 0;
	xfer.rxSize = 3;
	xfer.rxBuffer = manID;
	xfer.baseAddress = ctx->instance;
	xfer.operation = kFlexSpiOperation_Read;
	xfer.seqId = READ_JEDEC_ID_SEQ_ID;
	xfer.seqNum = 1;
	xfer.isParallelModeEnable = 0;

	return flexspi_norFlashExecuteSeq(ctx->instance, &xfer);
}


/* Read/write/erase operation via ROM API */

static int flashdrv_isValidAddress(flash_context_t *context, u32 addr, u32 size)
{
	if ((addr + size) <= context->properties.size)
		return 0;

	return 1;
}


static void flashdrv_syncCtx(flash_context_t *ctx)
{
	int i;
	u32 dstAddr;
	const u32 *src;
	u32 initAddr = ctx->address + ctx->sectorID * ctx->properties.sector_size;
	const u32 pagesNumber = ctx->properties.sector_size / ctx->properties.page_size;

	if (ctx->counter == 0)
		return;

	for (i = 0; i < pagesNumber; ++i) {
		dstAddr = ctx->sectorID * ctx->properties.sector_size + i * ctx->properties.page_size;
		src = (const u32 *)(ctx->buff + i * ctx->properties.page_size);

		flexspi_norFlashPageProgram(ctx->instance, &ctx->config, dstAddr, src);
	}

	hal_invalDCacheAddr(initAddr, ctx->properties.sector_size);

	ctx->counter = 0;
	ctx->sectorID = -1;
}


static s32 flashdrv_readData(flash_context_t *ctx, u32 offset, char *buff, u32 size)
{
	if (flashdrv_isValidAddress(ctx, offset, size))
		return -EINVAL;

	if (flexspi_norFlashRead(ctx->instance, &ctx->config, buff, offset, size) < 0)
		return -EINVAL;

	return size;
}


static s32 flashdrv_bufferedPagesWrite(flash_context_t *ctx, u32 offset, const char *buff, u32 size)
{
	u32 pageAddr;
	u16 sector_id;
	u32 savedBytes = 0;

	if (size % ctx->properties.page_size)
		return -EINVAL;

	if (flashdrv_isValidAddress(ctx, offset, size))
		return -EINVAL;

	while (savedBytes < size) {
		pageAddr = offset + savedBytes;
		sector_id = pageAddr / ctx->properties.sector_size;

		/* If sector_id has changed, data from previous sector have to be saved and new sector is read. */
		if (sector_id != ctx->sectorID) {
			flashdrv_syncCtx(ctx);

			if (flashdrv_readData(ctx, ctx->properties.sector_size * sector_id, ctx->buff, ctx->properties.sector_size) <= 0)
				return savedBytes;

			if (flexspi_norFlashErase(ctx->instance, &ctx->config, ctx->properties.sector_size * sector_id, ctx->properties.sector_size) != 0)
				return savedBytes;

			hal_invalDCacheAddr(ctx->address + ctx->properties.sector_size * sector_id, ctx->properties.sector_size);

			ctx->sectorID = sector_id;
			ctx->counter = offset - ctx->properties.sector_size * ctx->sectorID;
		}

		hal_memcpy(ctx->buff + ctx->counter, buff + savedBytes, ctx->properties.page_size);

		savedBytes += ctx->properties.page_size;
		ctx->counter += ctx->properties.page_size;

		/* Save filled buffer */
		if (ctx->counter >= ctx->properties.sector_size)
			flashdrv_syncCtx(ctx);
	}

	return size;
}


static int flashdrv_defineFlexSPI(flash_context_t *ctx)
{
	switch (ctx->address) {
		case FLASH_FLEXSPI1:
			ctx->instance = FLASH_FLEXSPI1_INSTANCE;
			ctx->maxSize = FLASH_SIZE_FLEXSPI1;
			ctx->option.option0 = FLASH_FLEXSPI1_QSPI_FREQ;
			ctx->option.option1 = 0;
			break;

		case FLASH_FLEXSPI2:
			ctx->instance = FLASH_FLEXSPI2_INSTANCE;
			ctx->maxSize = FLASH_SIZE_FLEXSPI2;
			ctx->option.option0 = FLASH_FLEXSPI2_QSPI_FREQ;
			ctx->option.option1 = 0;
			break;

		default:
			return -EINVAL;
	}

	return EOK;
}


/* Device interface */
static ssize_t flashdrv_read(unsigned int minor, addr_t offs, u8 *buff, unsigned int len, unsigned int timeout)
{
	if (minor >= FLASH_NO)
		return -EINVAL;

	hal_memcpy(buff, (void *)(flashdrv_common.ctx[minor].address + offs), len);
	return len;
}


static ssize_t flashdrv_write(unsigned int minor, addr_t offs, const u8 *buff, unsigned int len)
{
	addr_t bOffs = 0;
	flash_context_t *ctx;
	size_t size, buffSz = sizeof(flashdrv_common.buff);

	size = len;
	if (!len)
		return EOK;

	if (minor >= FLASH_NO || offs % buffSz)
		return -EINVAL;

	ctx = &flashdrv_common.ctx[minor];

	while (size) {
		if (size < buffSz) {
			hal_memcpy(flashdrv_common.buff, buff + bOffs, size);
			hal_memset(flashdrv_common.buff + size, 0xff, buffSz - size);
			if (flashdrv_bufferedPagesWrite(ctx, offs + bOffs, (const char *)flashdrv_common.buff, buffSz) < 0)
				return -EINVAL;
			size = 0;
			break;
		}

		if (flashdrv_bufferedPagesWrite(ctx, offs + bOffs, (const char *)(buff + bOffs), buffSz) < 0)
			return -EINVAL;

		bOffs += buffSz;
		size -= buffSz;
	}

	return len - size;
}


static int flashdrv_done(unsigned int minor)
{
	if (minor >= FLASH_NO)
		return -EINVAL;

	/* TBD */

	return EOK;
}


static int flashdrv_sync(unsigned int minor)
{
	if (minor >= FLASH_NO)
		return -EINVAL;

	flashdrv_syncCtx(&flashdrv_common.ctx[minor]);

	return EOK;
}


static int flashdrv_map(unsigned int minor, addr_t addr, size_t sz, int mode, addr_t memaddr, size_t memsz, int memmode, addr_t *a)
{
	size_t fSz;
	addr_t fStart;

	if (minor >= FLASH_NO)
		return -EINVAL;

	fStart = flashdrv_common.ctx[minor].address;
	fSz = flashdrv_common.ctx[minor].properties.size;

	/* Check if region is located on flash */
	if ((addr + sz) > fSz)
		return -EINVAL;

	/* Check if flash is mappable to map region */
	if (fStart <= memaddr && (fStart + fSz) >= (memaddr + memsz)) {
		*a = fStart;
		return dev_isMappable;
	}

	/* Device mode cannot be higher than map mode to copy data */
	if ((mode & memmode) != mode)
		return -EINVAL;

	/* Data can be copied from device to map */
	return dev_isNotMappable;
}


static int flashdrv_init(unsigned int minor)
{
	u32 pc;
	int res = EOK;
	flash_context_t *ctx;

	if (minor >= FLASH_NO)
		return -EINVAL;

	/* TODO: move information about dn to device tree module */
	if (minor == 0 && FLASH_FLEXSPI1_MOUNTED) {
		ctx = &flashdrv_common.ctx[minor];
		ctx->address = FLASH_FLEXSPI1;
	}
	else if (minor == 1 && FLASH_FLEXSPI2_MOUNTED) {
		ctx = &flashdrv_common.ctx[minor];
		ctx->address = FLASH_FLEXSPI2;
	}
	else {
		return -EINVAL;
	}

	ctx->sectorID = -1;
	ctx->counter = 0;

	ctx->config.ipcmdSerialClkFreq = 8;
	ctx->config.mem.serialClkFreq = 8;
	ctx->config.mem.sflashPadType = 4;

	if ((res = flashdrv_defineFlexSPI(ctx)) < 0)
		return res;

	/* Don't try to initialize FlexSPI if we're already XIP from it */
	__asm__ volatile ("mov %0, pc" :"=r"(pc));
	if (!(pc >= ctx->address && pc < ctx->address + ctx->maxSize)) {
		if (flexspi_norGetConfig(ctx->instance, &ctx->config, &ctx->option) != 0)
			return -EINVAL;

		if (flexspi_norFlashInit(ctx->instance, &ctx->config) != 0)
			return -EINVAL;
	}
	else {
		hal_memcpy((void *)&ctx->config, &_fcfb, sizeof(ctx->config));
	}

	if (flashdrv_getVendorID(ctx, &ctx->flashID) != 0)
		return -EINVAL;

	if (flashcfg_getCfg(ctx) != 0)
		return -EINVAL;

	lib_printf("\ndev/flash: Initializing flash(%d.%d)", DEV_FLASH, minor);

	return res;
}


__attribute__((constructor)) static void flashdrv_reg(void)
{
	static const dev_handler_t h = {
		.init = flashdrv_init,
		.done = flashdrv_done,
		.read = flashdrv_read,
		.write = flashdrv_write,
		.sync = flashdrv_sync,
		.map = flashdrv_map
	};

	devs_register(DEV_FLASH, FLASH_NO, &h);
}
