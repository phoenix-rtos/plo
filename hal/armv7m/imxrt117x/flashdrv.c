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

#include "../hal.h"
#include "../timer.h"
#include "../errors.h"

#include "lut.h"
#include "rom_api.h"
#include "flashdrv.h"
#include "flashcfg.h"


#define QSPI_FREQ_133MHZ 0xc0000007


/* Flash config commands */

static int flashdrv_setWEL(flash_context_t *ctx, u32 dstAddr)
{
	flexspi_xfer_t xfer;

	xfer.baseAddress = dstAddr;
	xfer.operation = kFlexSpiOperation_Command;
	xfer.seqId = WRITE_ENABLE_SEQ_ID;
	xfer.seqNum = 1;
	xfer.isParallelModeEnable = 0;

	return -flexspi_norFlashExecuteSeq(ctx->instance, &xfer);
}


static int flashdrv_writeBytes(flash_context_t *ctx, u32 dstAddr, u32 *src, u32 size)
{
	flexspi_xfer_t xfer;

	xfer.txSize = size;
	xfer.txBuffer = src;
	xfer.baseAddress = dstAddr;
	xfer.operation = kFlexSpiOperation_Write;
	xfer.seqId = PAGE_PROGRAM_SEQ_ID;
	xfer.seqNum = 1;

	return -flexspi_norFlashExecuteSeq(ctx->instance, &xfer);
}


static int flashdrv_waitBusBusy(flash_context_t *ctx)
{
	int err = ERR_NONE;
	u32 buff;
	u8 retrans;
	flexspi_xfer_t xfer;
	const u8 MAX_RETRANS = 8;

	retrans = 0;
	xfer.rxSize = 4;
	xfer.rxBuffer = &buff;
	xfer.baseAddress = ctx->instance;
	xfer.operation = kFlexSpiOperation_Read;
	xfer.seqId = READ_STATUS_REG_SEQ_ID;
	xfer.seqNum = 1;
	xfer.isParallelModeEnable = 0;

	do {
		err = flexspi_norFlashExecuteSeq(ctx->instance, &xfer);
		timer_wait(100, 0, NULL, 0);
	} while ((buff & (0x1 << 1)) && (++retrans < MAX_RETRANS) && (err == 0));

	return err;
}


static int flashdrv_getVendorID(flash_context_t *ctx, u32 *manID)
{
	flexspi_xfer_t xfer;

	/* READ JEDEC ID*/
	ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID] = LUT_INSTR(LUT_CMD_CMD, LUT_PAD1, FLASH_SPANSION_CMD_RDID, LUT_CMD_READ, LUT_PAD1, FLASH_SPANSION_CMD_WRDI); // 0x2404049f
	ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID + 1] = 0;
	ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID + 2] = 0;
	ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID + 3] = 0;
	flexspi_norFlashUpdateLUT(ctx->instance, READ_JEDEC_ID_SEQ_ID, (const u32 *)&ctx->config.mem.lut[4 * READ_JEDEC_ID_SEQ_ID], 1);

	xfer.txBuffer = NULL;
	xfer.txSize = 0;
	xfer.rxSize = 3;
	xfer.rxBuffer = manID;
	xfer.baseAddress = 0;
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


s32 flashdrv_readData(flash_context_t *ctx, u32 offset, char *buff, u32 size)
{
	if (flashdrv_isValidAddress(ctx, offset, size))
		return -1;

	if (flexspi_norFlashRead(ctx->instance, &ctx->config, buff, offset, size) < 0)
		return -1;

	return size;
}


s32 flashdrv_directBytesWrite(flash_context_t *ctx, u32 offset, const char *buff, u32 size)
{
	int err;
	u32 chunk;
	u32 len = size;

	while (len) {
		if ((chunk = ctx->properties.page_size - (offset & 0xff)) > len)
			chunk = len;

		if ((err = flashdrv_waitBusBusy(ctx)) < 0)
			return -1;

		if ((err = flashdrv_setWEL(ctx, offset)) < 0)
			return -1;

		if ((err = flashdrv_writeBytes(ctx, offset, (u32 *)buff, chunk)) < 0)
			return -1;

		if ((err = flashdrv_waitBusBusy(ctx)) < 0)
			return -1;

		offset += chunk;
		len -= chunk;
		buff = (char *)buff + chunk;
	}

	return size - len;
}


s32 flashdrv_bufferedPagesWrite(flash_context_t *ctx, u32 offset, const char *buff, u32 size)
{
	u32 pageAddr;
	u16 sector_id;
	u32 savedBytes = 0;

	if (size % ctx->properties.page_size)
		return -1;

	if (flashdrv_isValidAddress(ctx, offset, size))
		return -1;

	while (savedBytes < size) {
		pageAddr = offset + savedBytes;
		sector_id = pageAddr / ctx->properties.sector_size;

		/* If sector_id has changed, data from previous sector have to be saved and new sector is read. */
		if (sector_id != ctx->sectorID) {
			flashdrv_sync(ctx);

			if (flashdrv_readData(ctx, ctx->properties.sector_size * sector_id, ctx->buff, ctx->properties.sector_size) <= 0)
				return savedBytes;

			if (flexspi_norFlashErase(ctx->instance, &ctx->config, ctx->properties.sector_size * sector_id, ctx->properties.sector_size) != 0)
				return savedBytes;

			ctx->sectorID = sector_id;
			ctx->counter = offset - ctx->properties.sector_size * ctx->sectorID;
		}

		hal_memcpy(ctx->buff + ctx->counter, buff + savedBytes, ctx->properties.page_size);

		savedBytes += ctx->properties.page_size;
		ctx->counter += ctx->properties.page_size;

		/* Save filled buffer */
		if (ctx->counter >= ctx->properties.sector_size)
			flashdrv_sync(ctx);
	}

	return size;
}


int flashdrv_chipErase(flash_context_t *ctx)
{
	return flexspi_norFlashEraseAll(ctx->instance, &ctx->config);
}


int flashdrv_sectorErase(flash_context_t *ctx, u32 offset)
{
	if (offset % ctx->properties.sector_size)
		return -1;

	return flexspi_norFlashErase(ctx->instance, &ctx->config, offset, ctx->properties.sector_size);
}


void flashdrv_sync(flash_context_t *ctx)
{
	int i;
	u32 dstAddr;
	const u32 *src;
	const u32 pagesNumber = ctx->properties.sector_size / ctx->properties.page_size;

	if (ctx->counter == 0)
		return;

	for (i = 0; i < pagesNumber; ++i) {
		dstAddr = ctx->sectorID * ctx->properties.sector_size + i * ctx->properties.page_size;
		src = (const u32 *)(ctx->buff + i * ctx->properties.page_size);

		flexspi_norFlashPageProgram(ctx->instance, &ctx->config, dstAddr, src);
	}

	ctx->counter = 0;
	ctx->sectorID = -1;
}


/* Init functions */


int flashdrv_init(flash_context_t *ctx)
{
	int res = ERR_NONE;
	u32 pc;

	ctx->sectorID = -1;
	ctx->counter = 0;

	ctx->config.ipcmdSerialClkFreq = 8;
	ctx->config.mem.serialClkFreq = 8;
	ctx->config.mem.sflashPadType = 4;

	ctx->instance = 1;
	ctx->option.option0 = 0xc0000007;
	ctx->option.option1 = 0x00000000;

	/* Don't try to initialize FlexSPI if we're already XIP from it */
	__asm__ volatile ("mov %0, pc" :"=r"(pc));
	if (pc >= 0x30000000 && pc < 0x40000000)
		return 0;

	if (flexspi_norGetConfig(ctx->instance, &ctx->config, &ctx->option) != 0)
		return ERR_ARG;

	if (flexspi_norFlashInit(ctx->instance, &ctx->config) != 0)
		return ERR_ARG;

	if (flashdrv_getVendorID(ctx, &ctx->flashID) != 0)
		return ERR_ARG;

	if (flashcfg_getCfg(ctx) != 0)
		return ERR_ARG;

	return res;
}


void flashdrv_contextDestroy(flash_context_t *ctx)
{
	ctx->address = 0;
	flashdrv_sync(ctx);
}
