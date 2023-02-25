/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX 6ull NOR flash device driver
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski, Hubert Badocha
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _LUTTABLES_H_
#define _LUTTABLES_H_

#define LUTSZ_GENERIC (4 * 10)

static const u32 lutGeneric[LUTSZ_GENERIC] = {
	/* Read Fast Quad (3-byte address) */
	/* TODO This instruction is not tested as it had to be modified from i.MX RT driver as MODE8 is not available on imx6ull. */
	[4 * qspi_readData + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_QIOR, lutCmdADDR_SDR, lutPad4, 0x18),
	[4 * qspi_readData + 1] = LUT_SEQ(lutCmdMODE4_SDR, lutPad4, 0x00, lutCmdMODE4_SDR, lutPad4, 0x00),
	[4 * qspi_readData + 2] = LUT_SEQ(lutCmdDUMMY, lutPad4, 0x04, lutCmdREAD_SDR, lutPad4, 0x04),
	[4 * qspi_readData + 3] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),

	/* Read Status Register */
	[4 * qspi_readStatus + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_RDSR1, lutCmdREAD_SDR, lutPad1, 0x04),
	[4 * qspi_readStatus + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_readStatus + 2] = 0,
	[4 * qspi_readStatus + 3] = 0,

	/* Write Status Register */
	[4 * qspi_writeStatus + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_WRR1, lutCmdWRITE_SDR, lutPad1, 0x04),
	[4 * qspi_writeStatus + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_writeStatus + 2] = 0,
	[4 * qspi_writeStatus + 3] = 0,

	/* Write Enable */
	[4 * qspi_writeEnable + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_WREN, lutCmdSTOP, lutPad1, 0),
	[4 * qspi_writeEnable + 1] = 0,
	[4 * qspi_writeEnable + 2] = 0,
	[4 * qspi_writeEnable + 3] = 0,

	/* Write Disable */
	[4 * qspi_writeDisable + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_WRDI, lutCmdSTOP, lutPad1, 0),
	[4 * qspi_writeDisable + 1] = 0,
	[4 * qspi_writeDisable + 2] = 0,
	[4 * qspi_writeDisable + 3] = 0,

	/* Sector Erase (3-byte address) */
	[4 * qspi_eraseSector + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_P4E, lutCmdADDR_SDR, lutPad1, 0x18),
	[4 * qspi_eraseSector + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_eraseSector + 2] = 0,
	[4 * qspi_eraseSector + 3] = 0,

	/* Block Erase (3-byte address) */
	[4 * qspi_eraseBlock + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_SE, lutCmdADDR_SDR, lutPad1, 0x18),
	[4 * qspi_eraseBlock + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_eraseBlock + 2] = 0,
	[4 * qspi_eraseBlock + 3] = 0,

	/* Chip Erase */
	[4 * qspi_eraseChip + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_CE, lutCmdSTOP, lutPad1, 0),
	[4 * qspi_eraseChip + 1] = 0,
	[4 * qspi_eraseChip + 2] = 0,
	[4 * qspi_eraseChip + 3] = 0,

	/* Quad Input Page Program (3-byte address) */
	[4 * qspi_programQPP + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_QPP, lutCmdADDR_SDR, lutPad1, 0x18),
	[4 * qspi_programQPP + 1] = LUT_SEQ(lutCmdWRITE_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	[4 * qspi_programQPP + 2] = 0,
	[4 * qspi_programQPP + 3] = 0,

	[4 * qspi_readID + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_RDID, lutCmdREAD_SDR, lutPad1, 0x04),
	[4 * qspi_readID + 1] = 0,
	[4 * qspi_readID + 2] = 0,
	[4 * qspi_readID + 3] = 0,
};


#define LUTSZ_MICRON (4 * 10)

static const u32 lutMicron[LUTSZ_MICRON] = {
	/* Read Fast Quad (4-byte address) */
	[4 * qspi_readData + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_4QIOR, lutCmdADDR_SDR, lutPad4, 0x20),
	[4 * qspi_readData + 1] = LUT_SEQ(lutCmdDUMMY, lutPad4, 0x0a, lutCmdREAD_SDR, lutPad4, 0x04),
	[4 * qspi_readData + 2] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_readData + 3] = 0,

	/* Read Status Register */
	[4 * qspi_readStatus + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_RDSR1, lutCmdREAD_SDR, lutPad1, 0x04),
	[4 * qspi_readStatus + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_readStatus + 1] = 0,
	[4 * qspi_readStatus + 1] = 0,

	/* Write Status Register */
	[4 * qspi_writeStatus + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_WRR1, lutCmdWRITE_SDR, lutPad1, 0x04),
	[4 * qspi_writeStatus + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_writeStatus + 2] = 0,
	[4 * qspi_writeStatus + 3] = 0,

	/* Write Enable */
	[4 * qspi_writeEnable + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_WREN, lutCmdSTOP, lutPad1, 0),
	[4 * qspi_writeEnable + 1] = 0,
	[4 * qspi_writeEnable + 2] = 0,
	[4 * qspi_writeEnable + 3] = 0,

	/* Write Disable */
	[4 * qspi_writeDisable + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_WRDI, lutCmdSTOP, lutPad1, 0),
	[4 * qspi_writeDisable + 1] = 0,
	[4 * qspi_writeDisable + 2] = 0,
	[4 * qspi_writeDisable + 3] = 0,

	/* Sector Erase (4-byte address) */
	[4 * qspi_eraseSector + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_4P4E, lutCmdADDR_SDR, lutPad1, 0x20),
	[4 * qspi_eraseSector + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_eraseSector + 2] = 0,
	[4 * qspi_eraseSector + 3] = 0,

	/* Block Erase (4-byte address) */
	[4 * qspi_eraseBlock + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_4SE, lutCmdADDR_SDR, lutPad1, 0x20),
	[4 * qspi_eraseBlock + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * qspi_eraseBlock + 3] = 0,
	[4 * qspi_eraseBlock + 4] = 0,

	/* Chip Erase */
	[4 * qspi_eraseChip + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_BE, lutCmdSTOP, lutPad1, 0),
	[4 * qspi_eraseChip + 1] = 0,
	[4 * qspi_eraseChip + 2] = 0,
	[4 * qspi_eraseChip + 3] = 0,

	/* Quad Input Page Program (4-byte address) */
	[4 * qspi_programQPP + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_4QEPP, lutCmdADDR_SDR, lutPad4, 0x20),
	[4 * qspi_programQPP + 1] = LUT_SEQ(lutCmdWRITE_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	[4 * qspi_programQPP + 2] = 0,
	[4 * qspi_programQPP + 3] = 0,

	[4 * qspi_readID + 0] = LUT_SEQ(lutCmd, lutPad1, FLASH_CMD_RDID, lutCmdREAD_SDR, lutPad1, 0x04),
	[4 * qspi_readID + 1] = 0,
	[4 * qspi_readID + 2] = 0,
	[4 * qspi_readID + 3] = 0,
};


#endif /* _LUTTABLES_H_ */
