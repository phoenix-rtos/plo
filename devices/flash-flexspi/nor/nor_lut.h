/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT NOR flash device driver
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Gerard Swiderski
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
	[4 * fspi_readData + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_QIOR, lutCmdRADDR_SDR, lutPad4, 0x18),
	[4 * fspi_readData + 1] = LUT_SEQ(lutCmdMODE8_SDR, lutPad4, 0x00, lutCmdDUMMY_SDR, lutPad4, 0x04),
	[4 * fspi_readData + 2] = LUT_SEQ(lutCmdREAD_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_readData + 3] = 0,

	/* Read Status Register */
	[4 * fspi_readStatus + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_RDSR1, lutCmdREAD_SDR, lutPad1, 0x04),
	[4 * fspi_readStatus + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_readStatus + 2] = 0,
	[4 * fspi_readStatus + 3] = 0,

	/* Write Status Register */
	[4 * fspi_writeStatus + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WRR1, lutCmdWRITE_SDR, lutPad1, 0x04),
	[4 * fspi_writeStatus + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_writeStatus + 2] = 0,
	[4 * fspi_writeStatus + 3] = 0,

	/* Write Enable */
	[4 * fspi_writeEnable + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WREN, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_writeEnable + 1] = 0,
	[4 * fspi_writeEnable + 2] = 0,
	[4 * fspi_writeEnable + 3] = 0,

	/* Write Disable */
	[4 * fspi_writeDisable + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WRDI, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_writeDisable + 1] = 0,
	[4 * fspi_writeDisable + 2] = 0,
	[4 * fspi_writeDisable + 3] = 0,

	/* Sector Erase (3-byte address) */
	[4 * fspi_eraseSector + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_P4E, lutCmdRADDR_SDR, lutPad1, 0x18),
	[4 * fspi_eraseSector + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_eraseSector + 2] = 0,
	[4 * fspi_eraseSector + 3] = 0,

	/* Block Erase (3-byte address) */
	[4 * fspi_eraseBlock + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_SE, lutCmdRADDR_SDR, lutPad1, 0x18),
	[4 * fspi_eraseBlock + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_eraseBlock + 2] = 0,
	[4 * fspi_eraseBlock + 3] = 0,

	/* Chip Erase */
	[4 * fspi_eraseChip + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_CE, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_eraseChip + 1] = 0,
	[4 * fspi_eraseChip + 2] = 0,
	[4 * fspi_eraseChip + 3] = 0,

	/* Quad Input Page Program (3-byte address) */
	[4 * fspi_programQPP + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_QPP, lutCmdRADDR_SDR, lutPad1, 0x18),
	[4 * fspi_programQPP + 1] = LUT_SEQ(lutCmdWRITE_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_programQPP + 2] = 0,
	[4 * fspi_programQPP + 3] = 0,

	[4 * fspi_readID + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_RDID, lutCmdREAD_SDR, lutPad1, 0x04),
	[4 * fspi_readID + 1] = 0,
	[4 * fspi_readID + 2] = 0,
	[4 * fspi_readID + 3] = 0,
};


#define LUTSZ_MICRON (4 * 10)

static const u32 lutMicron[LUTSZ_MICRON] = {
	/* Read Fast Quad (4-byte address) */
	[4 * fspi_readData + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4QIOR, lutCmdRADDR_SDR, lutPad4, 0x20),
	[4 * fspi_readData + 1] = LUT_SEQ(lutCmdDUMMY_SDR, lutPad4, 0x0a, lutCmdREAD_SDR, lutPad4, 0x04),
	[4 * fspi_readData + 2] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_readData + 3] = 0,

	/* Read Status Register */
	[4 * fspi_readStatus + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_RDSR1, lutCmdREAD_SDR, lutPad1, 0x04),
	[4 * fspi_readStatus + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_readStatus + 1] = 0,
	[4 * fspi_readStatus + 1] = 0,

	/* Write Status Register */
	[4 * fspi_writeStatus + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WRR1, lutCmdWRITE_SDR, lutPad1, 0x04),
	[4 * fspi_writeStatus + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_writeStatus + 2] = 0,
	[4 * fspi_writeStatus + 3] = 0,

	/* Write Enable */
	[4 * fspi_writeEnable + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WREN, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_writeEnable + 1] = 0,
	[4 * fspi_writeEnable + 2] = 0,
	[4 * fspi_writeEnable + 3] = 0,

	/* Write Disable */
	[4 * fspi_writeDisable + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WRDI, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_writeDisable + 1] = 0,
	[4 * fspi_writeDisable + 2] = 0,
	[4 * fspi_writeDisable + 3] = 0,

	/* Sector Erase (4-byte address) */
	[4 * fspi_eraseSector + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4P4E, lutCmdRADDR_SDR, lutPad1, 0x20),
	[4 * fspi_eraseSector + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_eraseSector + 2] = 0,
	[4 * fspi_eraseSector + 3] = 0,

	/* Block Erase (4-byte address) */
	[4 * fspi_eraseBlock + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4SE, lutCmdRADDR_SDR, lutPad1, 0x20),
	[4 * fspi_eraseBlock + 1] = LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	[4 * fspi_eraseBlock + 3] = 0,
	[4 * fspi_eraseBlock + 4] = 0,

	/* Chip Erase */
	[4 * fspi_eraseChip + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_BE, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_eraseChip + 1] = 0,
	[4 * fspi_eraseChip + 2] = 0,
	[4 * fspi_eraseChip + 3] = 0,

	/* Quad Input Page Program (4-byte address) */
	[4 * fspi_programQPP + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4QEPP, lutCmdRADDR_SDR, lutPad4, 0x20),
	[4 * fspi_programQPP + 1] = LUT_SEQ(lutCmdWRITE_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	[4 * fspi_programQPP + 2] = 0,
	[4 * fspi_programQPP + 3] = 0,

	[4 * fspi_readID + 0] = LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_RDID, lutCmdREAD_SDR, lutPad1, 0x04),
	[4 * fspi_readID + 1] = 0,
	[4 * fspi_readID + 2] = 0,
	[4 * fspi_readID + 3] = 0,
};


#endif /* _LUTTABLES_H_ */
