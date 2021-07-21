/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT117x ROM API driver for FlexSPI
 *
 * Copyright 2019, 2020 Phoenix Systems
 * Author: Hubert Buczynski, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "romapi.h"


#define FLEXSPI_DRIVER_API_ADDRESS 0x021001c


typedef struct {
	u32 version;
	s32 (*init)(u32 instance, flexspi_norConfig_t *config);
	s32 (*page_program)(u32 instance, flexspi_norConfig_t *config, u32 dstAddr, const u32 *src);
	s32 (*erase_all)(u32 instance, flexspi_norConfig_t *config);
	s32 (*erase)(u32 instance, flexspi_norConfig_t *config, u32 start, u32 length);
	s32 (*read)(u32 instance, flexspi_norConfig_t *config, u32 *dst, u32 start, u32 bytes);
	void (*clear_cache)(u32 instance);
	s32 (*xfer)(u32 instance, flexspi_xfer_t *xfer);
	s32 (*update_lut)(u32 instance, u32 seqIndex, const u32 *lutBase, u32 numberOfSeq);
	s32 (*get_config)(u32 instance, flexspi_norConfig_t *config, serial_norConfigOption_t *option);
	s32 (*erase_sector)(u32 instance, flexspi_norConfig_t *config, u32 address);
	s32 (*erase_block)(u32 instance, flexspi_norConfig_t *config, u32 address);
	void (*hw_reset)(u32 instance, u32 resetLogic);
	s32 (*wait_busy)(u32 instance, flexspi_norConfig_t *config, char isParallelMode, u32 address);
	s32 (*set_clock_source)(u32 instance, u32 clockSrc);
	void (*config_clock)(u32 instance, u32 freqOption, u32 sampleClkMode);
} flexspi_nor_flash_driver_t;


typedef struct BootloaderTree {
	void (*runBootloader)(void *arg);
	int version;
	const char *copyright;
	const flexspi_nor_flash_driver_t *flexspiNorDriver;
} bootloader_tree_t;


static const bootloader_tree_t **volatile const bootloader_tree = (void *)FLEXSPI_DRIVER_API_ADDRESS;


int flexspi_norFlashInit(u32 instance, flexspi_norConfig_t *config)
{
	int res;

	hal_interruptsDisable();
	res = (*bootloader_tree)->flexspiNorDriver->init(instance, config);
	hal_interruptsEnable();

	return res;
}


int flexspi_norFlashPageProgram(u32 instance, flexspi_norConfig_t *config, u32 dstAddr, const u32 *src)
{
	int res;

	hal_interruptsDisable();
	res = (*bootloader_tree)->flexspiNorDriver->page_program(instance, config, dstAddr, src);
	hal_interruptsEnable();

	return res;
}


int flexspi_norFlashEraseAll(u32 instance, flexspi_norConfig_t *config)
{
	int res;

	hal_interruptsDisable();
	res = (*bootloader_tree)->flexspiNorDriver->erase_all(instance, config);
	hal_interruptsEnable();

	return res;
}


int flexspi_norGetConfig(u32 instance, flexspi_norConfig_t *config, serial_norConfigOption_t *option)
{
	int res;

	hal_interruptsDisable();
	res = (*bootloader_tree)->flexspiNorDriver->get_config(instance, config, option);
	hal_interruptsEnable();

	return res;
}


int flexspi_norFlashErase(u32 instance, flexspi_norConfig_t *config, u32 start, u32 length)
{
	int res;

	hal_interruptsDisable();
	res = (*bootloader_tree)->flexspiNorDriver->erase(instance, config, start, length);
	hal_interruptsEnable();

	return res;
}


int flexspi_norFlashRead(u32 instance, flexspi_norConfig_t *config, char *dst, u32 start, u32 bytes)
{
	int res;

	hal_interruptsDisable();
	res = (*bootloader_tree)->flexspiNorDriver->read(instance, config, (u32 *)dst, start, bytes);
	hal_interruptsEnable();

	return res;
}


int flexspi_norFlashUpdateLUT(u32 instance, u32 seqIndex, const u32 *lutBase, u32 seqNumber)
{
	int res;

	hal_interruptsDisable();
	res = (*bootloader_tree)->flexspiNorDriver->update_lut(instance, seqIndex, lutBase, seqNumber);
	hal_interruptsEnable();

	return res;
}


int flexspi_norFlashExecuteSeq(u32 instance, flexspi_xfer_t *xfer)
{
	int res;

	hal_interruptsDisable();
	res = (*bootloader_tree)->flexspiNorDriver->xfer(instance, xfer);
	hal_interruptsEnable();

	return res;
}
