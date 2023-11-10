/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GR712RC Flash on PROM interface driver
 *
 * Copyright 2023 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <lib/lib.h>
#include <hal/hal.h>

#include "cmds.h"
#include "flash.h"


/* Valid address located on flash - for executing commands */
#define FLASH_VALID_ADDR 0x1000

/* Flash status register */
#define STS_WSM_READY      (1 << 7) /* Write state machine ready */
#define STS_ERASE_SUSP     (1 << 6) /* Erase operation suspended */
#define STS_ERASE_CLR_LOCK (1 << 5) /* Erase/clear lock-bit error */
#define STS_PROG_SET_LOCK  (1 << 4) /* Program/set lock-bit error */
#define STS_PROG_VOLT_ERR  (1 << 3) /* Low programming voltage detected */
#define STS_PROG_SUSP      (1 << 2) /* Program operation suspended */
#define STS_DEV_PROTECT    (1 << 1) /* Block lock-bit detected */
#define STS_FULL_CHECK     (STS_ERASE_CLR_LOCK | STS_PROG_SET_LOCK | STS_PROG_VOLT_ERR | STS_DEV_PROTECT)
#define XSR_WRBUF_RDY      (1 << 7) /* Write buffer ready */


static const struct {
	const u8 id;
	const char *name;
} flash_vendors[] = {
	{ .id = 0x89, .name = "Intel" }
};


static u8 flash_readStatusCmd(void)
{
	*(vu8 *)FLASH_VALID_ADDR = FLASH_RD_STATUS;

	return hal_cpuLoadAlternate8((u8 *)FLASH_VALID_ADDR, ASI_CACHE_MISS);
}


static u8 flash_readStatusDirect(void)
{
	return hal_cpuLoadAlternate8((u8 *)FLASH_VALID_ADDR, ASI_CACHE_MISS);
}


static void flash_clearStatus(void)
{
	*(vu8 *)FLASH_VALID_ADDR = FLASH_CLR_STATUS;
}


static int flash_statusCheck(u8 status, const char *msg)
{
	int ret = ((status & STS_FULL_CHECK) == 0) ? EOK : -EIO;

	if (ret < 0) {
		log_error("\ndev/flash: %s error: status 0x%x\n", msg, status);
	}

	flash_clearStatus();

	return ret;
}


static int flash_statusWait(u8 (*readStatus)(void), time_t timeout)
{
	time_t start = hal_timerGet();

	while ((readStatus() & STS_WSM_READY) == 0) {
		if ((timeout > 0u) && (hal_timerGet() - start) > timeout) {
			return -ETIME;
		}
	}

	return EOK;
}


static int flash_clearLockBits(void)
{
	*(vu8 *)FLASH_VALID_ADDR = FLASH_UNLOCK_CYC1;
	*(vu8 *)FLASH_VALID_ADDR = FLASH_UNLOCK_CYC2;

	(void)flash_statusWait(flash_readStatusDirect, 0u);

	return flash_statusCheck(flash_readStatusCmd(), "clear lock bits");
}


static u16 flash_deserialize16(u16 value)
{
	return ((value & 0xff) << 8) | ((value >> 8) & 0xff);
}


int flash_writeBuffer(flash_device_t *dev, addr_t addr, const u8 *data, u8 len)
{
	size_t i;
	u8 xsr;

	for (i = 0; i < len; ++i) {
		if (data[i] != 0xff) {
			break;
		}
	}

	if (i == len) {
		return EOK;
	}

	do {
		*(vu8 *)addr = FLASH_WR_BUF;
		xsr = hal_cpuLoadAlternate8((u8 *)FLASH_VALID_ADDR, ASI_CACHE_MISS);
	} while ((xsr & XSR_WRBUF_RDY) == 0);

	*(vu8 *)addr = len - 1;

	for (i = 0; i < len; ++i) {
		*(vu8 *)(addr + i) = data[i];
	}

	*(vu8 *)addr = FLASH_WR_CONFIRM;

	if (flash_statusWait(flash_readStatusDirect,
			CFI_TIMEOUT_MAX_PROGRAM(dev->cfi.toutTypical.bufWrite, dev->cfi.toutMax.bufWrite)) < 0) {
		return -ETIME;
	}

	return flash_statusCheck(flash_readStatusDirect(), "write buffer");
}


int flash_blockErase(addr_t blkAddr, time_t timeout)
{
	*(vu8 *)blkAddr = FLASH_BE_CYC1;
	*(vu8 *)blkAddr = FLASH_WR_CONFIRM;

	if (flash_statusWait(flash_readStatusDirect, timeout) < 0) {
		return -ETIME;
	}

	return flash_statusCheck(flash_readStatusDirect(), "block erase");
}


void flash_read(flash_device_t *dev, addr_t offs, void *buff, size_t len)
{
	*(vu8 *)FLASH_VALID_ADDR = FLASH_RD_ARRAY;

	hal_cpuFlushDCache();

	hal_memcpy(buff, (void *)offs + dev->ahbStartAddr, len);
}


void flash_printInfo(int major, int minor, flash_cfi_t *cfi)
{
	size_t i;
	const char *vendor;
	for (i = 0; i < sizeof(flash_vendors) / sizeof(flash_vendors[0]); i++) {
		if (flash_vendors[i].id == cfi->vendorData[0]) {
			vendor = flash_vendors[i].name;
			break;
		}
	}

	if (vendor == NULL) {
		vendor = "Unknown";
	}

	lib_printf("\ndev/flash: configured %s %u MB flash(%d.%d)", vendor, CFI_SIZE(cfi->chipSz) / (1024 * 1024), major, minor);
}


static int flash_query(flash_cfi_t *cfi)
{
	size_t i;
	u8 *ptr = (u8 *)cfi;

	*(vu8 *)FLASH_VALID_ADDR = FLASH_RD_QUERY;
	for (i = 0; i < sizeof(flash_cfi_t); ++i) {
		ptr[i] = hal_cpuLoadAlternate8((u8 *)(i * 2), ASI_CACHE_MISS);
	}

	cfi->cmdSet1 = flash_deserialize16(cfi->cmdSet1);
	cfi->addrExt1 = flash_deserialize16(cfi->addrExt1);
	cfi->cmdSet2 = flash_deserialize16(cfi->cmdSet2);
	cfi->addrExt2 = flash_deserialize16(cfi->addrExt2);
	cfi->fdiDesc = flash_deserialize16(cfi->fdiDesc);
	cfi->bufSz = flash_deserialize16(cfi->bufSz);

	for (i = 0; i < cfi->regionCnt; i++) {
		cfi->regions[i].count = flash_deserialize16(cfi->regions[i].count);
		cfi->regions[i].size = flash_deserialize16(cfi->regions[i].size);
	}
	return EOK;
}


int flash_init(flash_device_t *dev)
{
	int ret;

	flash_clearLockBits();

	/* Reset */
	*(vu8 *)FLASH_VALID_ADDR = FLASH_RD_ARRAY;

	ret = flash_query(&dev->cfi);

	dev->blockSz = CFI_SIZE(dev->cfi.chipSz) / (dev->cfi.regions[0].count + 1);

	return ret;
}
