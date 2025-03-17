/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * AMD command set flash interface
 *
 * Copyright 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "flash.h"

#include <config.h>
#include <lib/lib.h>


/* GCC bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104657 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"


/* Valid offset on flash - for executing commands */
#define FLASH_VALID_OFFS 0x0
#define STS_FULL_CHECK   ((1 << 5) | (1 << 4) | (1 << 3) | (1 << 1))


__attribute__((section(".noxip"))) static void amd_unlockSequence(void)
{
	*(vu8 *)(ADDR_FLASH + 0x0aaa) = 0xaa;
	*(vu8 *)(ADDR_FLASH + 0x0555) = 0x55;
}


__attribute__((section(".noxip"))) static void amd_issueReset(void)
{
	*(vu8 *)(ADDR_FLASH + FLASH_VALID_OFFS) = AMD_CMD_RESET;
}


__attribute__((section(".noxip"))) static u8 amd_statusRead(void)
{
	*(vu8 *)(ADDR_FLASH + 0x0aaa) = CMD_RD_STATUS;

	return hal_cpuLoadAlternate8((u8 *)(ADDR_FLASH + FLASH_VALID_OFFS), ASI_MMU_BYPASS);
}


__attribute__((section(".noxip"))) static void amd_statusClear(void)
{
	*(vu8 *)(ADDR_FLASH + 0x0aaa) = AMD_CMD_CLR_STATUS;
}


__attribute__((section(".noxip"))) static int amd_statusCheck(const char *msg)
{
	int ret = EOK;
	u8 status = amd_statusRead();

	if ((status & STS_FULL_CHECK) != 0) {
		log_error("\ndev/flash: %s error: status 0x%x\n", msg, status);
		ret = -EIO;
	}

	return ret;
}


__attribute__((section(".noxip"))) static void amd_issueWriteBuffer(addr_t sectorAddr, addr_t programAddr, size_t len)
{
	(void)programAddr;

	amd_unlockSequence();

	*(vu8 *)(ADDR_FLASH + sectorAddr) = AMD_CMD_WR_BUF;
	*(vu8 *)(ADDR_FLASH + sectorAddr) = (len - 1) & 0xff;
}


__attribute__((section(".noxip"))) static void amd_issueWriteConfirm(addr_t sectorAddr)
{
	*(vu8 *)(ADDR_FLASH + sectorAddr) = AMD_CMD_WR_CONFIRM;
}


__attribute__((section(".noxip"))) static void amd_issueSectorErase(addr_t sectorAddr)
{
	amd_unlockSequence();
	*(vu8 *)(ADDR_FLASH + 0x0aaa) = AMD_CMD_BE_CYC1;

	amd_unlockSequence();
	*(vu8 *)(ADDR_FLASH + sectorAddr) = AMD_CMD_BE_CYC2;
}


__attribute__((section(".noxip"))) static void amd_issueChipErase(void)
{
	amd_unlockSequence();
	*(vu8 *)(ADDR_FLASH + 0x0aaa) = AMD_CMD_CE_CYC1;

	amd_unlockSequence();
	*(vu8 *)(ADDR_FLASH + 0x0aaa) = AMD_CMD_CE_CYC2;
}

__attribute__((section(".noxip"))) static void amd_enterQuery(addr_t sectorAddr)
{
	*(vu8 *)(ADDR_FLASH + sectorAddr + 0xaa) = CMD_RD_QUERY;
}


__attribute__((section(".noxip"))) static void amd_exitQuery(void)
{
	*(vu8 *)ADDR_FLASH = AMD_CMD_EXIT_QUERY;
}


#pragma GCC diagnostic pop


/* These structures must not reside on flash */
static cfi_ops_t ops = {
	.statusRead = amd_statusRead,
	.statusCheck = amd_statusCheck,
	.statusClear = amd_statusClear,
	.issueReset = amd_issueReset,
	.issueWriteBuffer = amd_issueWriteBuffer,
	.issueWriteConfirm = amd_issueWriteConfirm,
	.issueSectorErase = amd_issueSectorErase,
	.issueChipErase = amd_issueChipErase,
	.enterQuery = amd_enterQuery,
	.exitQuery = amd_exitQuery
};


static cfi_dev_t amd_devices[] = {
	{
		.name = "Infineon S29GL01/512T",
		.vendor = 0x01u,
		.device = 0x227eu,
		.chipWidth = 16,
		.statusRdyMask = (1 << 7),
		.usePolling = 1,
		.ops = &ops,
	}
};


void amd_register(void)
{
	size_t i;
	for (i = 0; i < (sizeof(amd_devices) / sizeof(amd_devices[0])); ++i) {
		flash_register(&amd_devices[i]);
	}
}
