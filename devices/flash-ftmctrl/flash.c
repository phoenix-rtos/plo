/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB FTMCTRL Flash driver
 *
 * Copyright 2023, 2024 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "flash.h"
#include "ftmctrl.h"

#include <config.h>

#include <lib/lib.h>
#include <hal/hal.h>


#define OFFSETOF(type, member) ((size_t)(&((type *)0)->member))

#define FLASH_DEVICES 2


typedef union {
	u8 b;
	u16 w;
} flash_word_t;


static struct {
	size_t nmodels;
	const cfi_dev_t *models[FLASH_DEVICES];
} flash_common;


__attribute__((section(".noxip"))) static int flash_statusPoll(flash_device_t *dev, const flash_word_t src, void *dst, time_t timeout)
{
	u16 val;
	int ready;
	time_t start = hal_timerGet();

	for (;;) {
		switch (ftmctrl_portWidth()) {
			case 8:
				ready = (src.b == hal_cpuLoadAlternate8(dst, ASI_MMU_BYPASS)) ? 1 : 0;
				break;

			case 16:
				val = hal_cpuLoadAlternate8(dst, ASI_MMU_BYPASS) | (hal_cpuLoadAlternate8(dst + 1, ASI_MMU_BYPASS) << 8);
				ready = (src.w == val) ? 1 : 0;
				break;

			default:
				return -EINVAL;
		}

		if (ready != 0) {
			break;
		}

		if ((timeout > 0u) && (hal_timerGet() - start) > timeout) {
			return -ETIME;
		}
	}

	return EOK;
}


__attribute__((section(".noxip"))) static int flash_statusWait(flash_device_t *dev, time_t timeout)
{
	time_t start = hal_timerGet();

	while ((dev->model->ops->statusRead() & dev->model->statusRdyMask) == 0) {
		if ((timeout > 0u) && (hal_timerGet() - start) > timeout) {
			return -ETIME;
		}
	}

	return EOK;
}


__attribute__((section(".noxip"))) static u16 flash_deserialize16(u16 value)
{
	return ((value & 0xff) << 8) | ((value >> 8) & 0xff);
}


__attribute__((section(".noxip"))) int flash_writeBuffer(flash_device_t *dev, addr_t addr, const u8 *data, size_t len, time_t timeout)
{
	int res, status;
	u16 val;
	flash_word_t word;
	const int portWidth = ftmctrl_portWidth();
	addr_t sectorAddr = flash_getSectorAddress(dev, addr);
	size_t i;

	for (i = 0; i < len; i += portWidth / 8) {
		if (portWidth == 8) {
			if (data[i] != 0xffu) {
				break;
			}
		}
		else if (portWidth == 16) {
			val = data[i] | (data[i + 1] << 8);
			if (val != 0xffffu) {
				break;
			}
		}
		else {
			return -EINVAL;
		}
	}


	len -= i;
	addr += i;
	data += i;

	if (len == 0) {
		return EOK;
	}

	dev->model->ops->issueWriteBuffer(sectorAddr, addr, len);

	i = 0;
	while (i < len) {
		switch (portWidth) {
			case 8:
				*(vu8 *)(ADDR_FLASH + addr + i) = data[i];
				i++;
				break;

			case 16:
				val = data[i] | (data[i + 1] << 8);
				*(vu16 *)(ADDR_FLASH + addr + i) = val;
				i += 2;
				break;

			default:
				break;
		}
	}

	dev->model->ops->issueWriteConfirm(sectorAddr);

	if (dev->model->usePolling != 0) {
		switch (portWidth) {
			case 8:
				word.b = data[len - 1];
				break;

			case 16:
				word.w = data[len - 2] | (data[len - 1] << 8);
				break;

			default:
				return -EINVAL;
		}

		res = flash_statusPoll(dev, word, (u8 *)(ADDR_FLASH + addr + len - (portWidth / 8)), timeout);
	}
	else {
		res = flash_statusWait(dev, timeout);
	}

	status = dev->model->ops->statusCheck("write buffer");

	dev->model->ops->statusClear();

	return (res == EOK) ? status : res;
}


__attribute__((section(".noxip"))) int flash_sectorErase(flash_device_t *dev, addr_t sectorAddr, time_t timeout)
{
	int res, status;
	flash_word_t word;
	dev->model->ops->issueSectorErase(sectorAddr);

	if (dev->model->usePolling != 0) {
		switch (ftmctrl_portWidth()) {
			case 8:
				word.b = 0xffu;
				break;

			case 16:
				word.w = 0xffffu;
				break;

			default:
				return -EINVAL;
		}

		res = flash_statusPoll(dev, word, (void *)(ADDR_FLASH + sectorAddr), timeout);
	}
	else {
		res = flash_statusWait(dev, timeout);
	}

	status = dev->model->ops->statusCheck("sector erase");

	dev->model->ops->statusClear();

	return (res == EOK) ? status : res;
}


__attribute__((section(".noxip"))) int flash_chipErase(flash_device_t *dev, time_t timeout)
{
	int res, status;
	if (dev->model->ops->issueChipErase == NULL) {
		return -ENOSYS;
	}

	dev->model->ops->issueChipErase();

	res = flash_statusWait(dev, timeout);

	status = dev->model->ops->statusCheck("chip erase");

	dev->model->ops->statusClear();

	return (res == EOK) ? status : res;
}


__attribute__((section(".noxip"))) void flash_read(flash_device_t *dev, addr_t offs, void *buff, size_t len)
{
	dev->model->ops->issueReset();

	hal_memcpy(buff, (void *)(ADDR_FLASH + offs), len);
}


void flash_printInfo(flash_device_t *dev, int major, int minor)
{
	lib_printf("\ndev/flash: configured %s %u MB flash(%d.%d)", dev->model->name, CFI_SIZE(dev->cfi.chipSz) / (1024 * 1024), major, minor);
}


__attribute__((section(".noxip"))) static void flash_reset(void)
{
	volatile int i;

	/* Issue both commands as we don't know yet which flash we have */
	*(vu8 *)ADDR_FLASH = AMD_CMD_RESET;

	/* Small delay */
	for (i = 0; i < 1000; i++) { }

	*(vu8 *)ADDR_FLASH = INTEL_CMD_RESET;
}


__attribute__((section(".noxip"))) static const cfi_dev_t *flash_query(cfi_info_t *cfi)
{
	size_t i, j;
	u8 buf[3];
	u8 *ptr = (u8 *)cfi;
	u16 device;
	const cfi_dev_t *model;
	const size_t offs = OFFSETOF(cfi_info_t, qry);

	for (i = 0; i < flash_common.nmodels; i++) {
		model = flash_common.models[i];
		flash_reset();
		model->ops->enterQuery(0x0);

		/* Check 'QRY' header */
		for (j = 0; j < sizeof(cfi->qry); ++j) {
			buf[j] = hal_cpuLoadAlternate8((u8 *)(ADDR_FLASH + (offs + j) * 2), ASI_MMU_BYPASS);
		}

		if ((buf[0] != 'Q') || (buf[1] != 'R') || (buf[2] != 'Y')) {
			continue;
		}

		for (j = 0; j < sizeof(cfi_info_t); ++j) {
			ptr[j] = hal_cpuLoadAlternate8((u8 *)(ADDR_FLASH + j * 2), ASI_MMU_BYPASS);
		}

		((u8 *)&device)[0] = cfi->vendorData[1];
		((u8 *)&device)[1] = cfi->vendorData[2];
		/* x8 flash */
		device = flash_deserialize16(device) & 0xff;

		if ((cfi->vendorData[0] != model->vendor) || (device != (model->device & 0xff))) {
			/* Command succeeded, but this is not the flash we're checking now */
			model->ops->exitQuery();
			continue;
		}

		cfi->cmdSet1 = flash_deserialize16(cfi->cmdSet1);
		cfi->addrExt1 = flash_deserialize16(cfi->addrExt1);
		cfi->cmdSet2 = flash_deserialize16(cfi->cmdSet2);
		cfi->addrExt2 = flash_deserialize16(cfi->addrExt2);
		cfi->fdiDesc = flash_deserialize16(cfi->fdiDesc);
		cfi->bufSz = flash_deserialize16(cfi->bufSz);

		for (j = 0; j < cfi->regionCnt; j++) {
			cfi->regions[j].count = flash_deserialize16(cfi->regions[j].count);
			cfi->regions[j].size = flash_deserialize16(cfi->regions[j].size);
		}

		model->ops->exitQuery();

		return model;
	}

	return NULL;
}


__attribute__((section(".noxip"))) int flash_init(flash_device_t *dev)
{
	const cfi_dev_t *model;

	model = flash_query(&dev->cfi);
	if (model == NULL) {
		return -ENODEV;
	}

	dev->model = model;
	dev->sectorSz = CFI_SIZE(dev->cfi.chipSz) / (dev->cfi.regions[0].count + 1);

	return 0;
}


void flash_register(const cfi_dev_t *dev)
{
	flash_common.models[flash_common.nmodels++] = dev;
}
