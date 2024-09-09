/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Intel command set flash interface
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

/* Valid offset on flash - for executing commands */
#define FLASH_VALID_OFFS 0x0
/* Flash status register */
#define STS_FULL_CHECK ((1 << 5) | (1 << 4) | (1 << 3) | (1 << 1))
#define XSR_WRBUF_RDY  (1 << 7) /* Write buffer ready */


__attribute__((section(".noxip"))) static u8 intel_statusRead(void)
{
	*(vu8 *)(ADDR_FLASH + FLASH_VALID_OFFS) = CMD_RD_STATUS;

	return hal_cpuLoadAlternate8((u8 *)(ADDR_FLASH + FLASH_VALID_OFFS), ASI_MMU_BYPASS);
}


__attribute__((section(".noxip"))) static void intel_statusClear(void)
{
	*(vu8 *)(ADDR_FLASH + FLASH_VALID_OFFS) = INTEL_CMD_CLR_STATUS;
}


__attribute__((section(".noxip"))) static int intel_statusCheck(const char *msg)
{
	int ret = EOK;
	u8 status = intel_statusRead();

	if ((status & STS_FULL_CHECK) != 0) {
		log_error("\ndev/flash: %s error: status 0x%x\n", msg, status);
		ret = -EIO;
	}

	intel_statusClear();

	return ret;
}


__attribute__((section(".noxip"))) static void intel_issueReset(void)
{
	*(vu8 *)(ADDR_FLASH + FLASH_VALID_OFFS) = INTEL_CMD_RESET;
}


__attribute__((section(".noxip"))) static void intel_issueWriteBuffer(addr_t sectorAddr, addr_t programAddr, size_t len)
{
	u8 xsr;
	(void)sectorAddr;

	do {
		*(vu8 *)(ADDR_FLASH + programAddr) = INTEL_CMD_WR_BUF;
		xsr = hal_cpuLoadAlternate8((u8 *)(ADDR_FLASH + FLASH_VALID_OFFS), ASI_MMU_BYPASS);
	} while ((xsr & XSR_WRBUF_RDY) == 0);

	*(vu8 *)(ADDR_FLASH + programAddr) = (len - 1) & 0xff;
}


__attribute__((section(".noxip"))) static void intel_issueWriteConfirm(addr_t sectorAddr)
{
	*(vu8 *)(ADDR_FLASH + sectorAddr) = INTEL_CMD_WR_CONFIRM;
}


__attribute__((section(".noxip"))) static void intel_issueSectorErase(addr_t sectorAddr)
{
	*(vu8 *)(ADDR_FLASH + sectorAddr) = INTEL_CMD_BE_CYC1;
	*(vu8 *)(ADDR_FLASH + sectorAddr) = INTEL_CMD_WR_CONFIRM;
}


__attribute__((section(".noxip"))) static void intel_enterQuery(addr_t addr)
{
	*(vu8 *)(ADDR_FLASH + addr) = CMD_RD_QUERY;
}


__attribute__((section(".noxip"))) static void intel_exitQuery(void)
{
}


/* These structures must not reside on flash */
static cfi_ops_t ops = {
	.statusRead = intel_statusRead,
	.statusCheck = intel_statusCheck,
	.statusClear = intel_statusClear,
	.issueReset = intel_issueReset,
	.issueWriteBuffer = intel_issueWriteBuffer,
	.issueWriteConfirm = intel_issueWriteConfirm,
	.issueSectorErase = intel_issueSectorErase,
	.issueChipErase = NULL, /* Not supported */
	.enterQuery = intel_enterQuery,
	.exitQuery = intel_exitQuery
};


static cfi_dev_t intel_devices[] = {
	{
		.name = "Intel JS28F640J3",
		.vendor = 0x89u,
		.device = 0x0017u,
		.chipWidth = 8,
		.statusRdyMask = (1 << 7),
		.usePolling = 0,
		.ops = &ops,
	}
};


void intel_register(void)
{
	size_t i;
	for (i = 0; i < (sizeof(intel_devices) / sizeof(intel_devices[0])); ++i) {
		flash_register(&intel_devices[i]);
	}
}
