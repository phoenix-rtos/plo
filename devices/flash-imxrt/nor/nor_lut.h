/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT NOR flash device driver
 *
 * Copyright 2021-2023 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _LUTTABLES_H_
#define _LUTTABLES_H_

#define NOR_LUTSEQSZ 4

/*
 * Generic NOR Command Sequences
 */

/* Read Jedec ID */
static const u32 seq_genericReadID[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_RDID, lutCmdREAD_SDR, lutPad1, 0x04),
	0, 0, 0
};

/* Read Fast Quad (3-byte address) */
static const u32 seq_genericReadData3Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_QIOR, lutCmdRADDR_SDR, lutPad4, 0x18),
	LUT_SEQ(lutCmdMODE8_SDR, lutPad4, 0x00, lutCmdDUMMY_SDR, lutPad4, 0x04),
	LUT_SEQ(lutCmdREAD_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	0
};

/* Read Fast Quad (4-byte address) */
static const u32 seq_genericReadData4Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4QIOR, lutCmdRADDR_SDR, lutPad4, 0x20),
	LUT_SEQ(lutCmdMODE8_SDR, lutPad4, 0x00, lutCmdDUMMY_SDR, lutPad4, 0x04),
	LUT_SEQ(lutCmdREAD_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	0
};

/* Read Status Register */
static const u32 seq_genericReadStatus[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_RDSR1, lutCmdREAD_SDR, lutPad1, 0x04),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0, 0
};

/* Write Status Register */
static const u32 seq_genericWriteStatus[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WRR1, lutCmdWRITE_SDR, lutPad1, 0x04),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0, 0
};

/* Write Enable */
static const u32 seq_genericWriteEnable[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WREN, lutCmdSTOP, lutPad1, 0),
	0, 0, 0
};

/* Write Disable */
static const u32 seq_genericWriteDisable[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_WRDI, lutCmdSTOP, lutPad1, 0),
	0, 0, 0
};

/* Sector Erase (3-byte address) */
static const u32 seq_genericEraseSector3Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_P4E, lutCmdRADDR_SDR, lutPad1, 0x18),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0, 0
};

/* Sector Erase (4-byte address) */
static const u32 seq_genericEraseSector4Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4P4E, lutCmdRADDR_SDR, lutPad1, 0x20),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0, 0
};

/* Block Erase (3-byte address) */
static const u32 seq_genericEraseBlock3Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_SE, lutCmdRADDR_SDR, lutPad1, 0x18),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0, 0
};

/* Block Erase (4-byte address) */
static const u32 seq_genericEraseBlock4Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4SE, lutCmdRADDR_SDR, lutPad1, 0x20),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0, 0
};

/* Chip Erase */
static const u32 seq_genericEraseChip[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_CE, lutCmdSTOP, lutPad1, 0),
	0, 0, 0
};

/* Quad Input Page Program (3-byte address) */
static const u32 seq_genericProgramQPP3Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_QPP, lutCmdRADDR_SDR, lutPad1, 0x18),
	LUT_SEQ(lutCmdWRITE_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	0, 0
};

/* Quad Input Page Program (4-byte address) */
static const u32 seq_genericProgramQPP4Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4QPP, lutCmdRADDR_SDR, lutPad1, 0x20),
	LUT_SEQ(lutCmdWRITE_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	0, 0
};


/*
 * Micron NOR dedicated Command Sequences
 */

/* Read Fast Quad (4-byte address) */
static const u32 seq_micronReadData[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4QIOR, lutCmdRADDR_SDR, lutPad4, 0x20),
	LUT_SEQ(lutCmdDUMMY_SDR, lutPad4, 0x0a, lutCmdREAD_SDR, lutPad4, 0x04),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0
};


/* Sector Erase (4-byte address) */
static const u32 seq_micronEraseSector[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4P4E, lutCmdRADDR_SDR, lutPad1, 0x20),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0, 0
};

/* Block Erase (4-byte address) */
static const u32 seq_micronEraseBlock[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4SE, lutCmdRADDR_SDR, lutPad1, 0x20),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0, 0
};

/* Chip Erase (Erase Bulk) Micron up to 512Mb */
static const u32 seq_micronEraseBulk[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_BE, lutCmdSTOP, lutPad1, 0),
	0, 0, 0
};

/* Die Erase (with address of stacked die) */
static const u32 seq_micronEraseDie[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_DE, lutCmdRADDR_SDR, lutPad1, 0x20),
	LUT_SEQ(lutCmdSTOP, lutPad1, 0, 0, 0, 0),
	0,
	0,
};

/* Quad Input Page Program (4-byte address) */
static const u32 seq_micronProgramQPP[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_4QEPP, lutCmdRADDR_SDR, lutPad4, 0x20),
	LUT_SEQ(lutCmdWRITE_SDR, lutPad4, 0x04, lutCmdSTOP, lutPad1, 0),
	0, 0
};

/* Enter 4-byte address mode */
static const u32 seq_micronEnter4Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_EN4B, lutCmdSTOP, lutPad1, 0),
	0, 0, 0
};

/* Exit 4-byte address mode */
static const u32 seq_micronExit4Byte[NOR_LUTSEQSZ] = {
	LUT_SEQ(lutCmd_SDR, lutPad1, FLASH_CMD_EX4B, lutCmdSTOP, lutPad1, 0),
	0, 0, 0
};


/* Generic chips: ISSI, Winbond, Macronix (3-byte address) */
static const u32 *lutGeneric3Byte[LUT_ENTRIES] = {
	[fspi_readData] = seq_genericReadData3Byte,
	[fspi_readStatus] = seq_genericReadStatus,
	[fspi_writeStatus] = seq_genericWriteStatus,
	[fspi_writeEnable] = seq_genericWriteEnable,
	[fspi_writeDisable] = seq_genericWriteDisable,
	[fspi_eraseSector] = seq_genericEraseSector3Byte,
	[fspi_eraseBlock] = seq_genericEraseBlock3Byte,
	[fspi_eraseChip] = seq_genericEraseChip,
	[fspi_programQPP] = seq_genericProgramQPP3Byte,
	[fspi_readID] = seq_genericReadID,
};

/* Generic chips: ISSI, Winbond, Macronix (4-byte address) */
static const u32 *lutGeneric4Byte[LUT_ENTRIES] = {
	[fspi_readData] = seq_genericReadData4Byte,
	[fspi_readStatus] = seq_genericReadStatus,
	[fspi_writeStatus] = seq_genericWriteStatus,
	[fspi_writeEnable] = seq_genericWriteEnable,
	[fspi_writeDisable] = seq_genericWriteDisable,
	[fspi_eraseSector] = seq_genericEraseSector4Byte,
	[fspi_eraseBlock] = seq_genericEraseBlock4Byte,
	[fspi_eraseChip] = seq_genericEraseChip,
	[fspi_programQPP] = seq_genericProgramQPP4Byte,
	[fspi_readID] = seq_genericReadID,
};

/* Micron chips with bulk erase <=512Mb */
static const u32 *lutMicronMono[LUT_ENTRIES] = {
	[fspi_readData] = seq_micronReadData,
	[fspi_readStatus] = seq_genericReadStatus,
	[fspi_writeStatus] = seq_genericWriteStatus,
	[fspi_writeEnable] = seq_genericWriteEnable,
	[fspi_writeDisable] = seq_genericWriteDisable,
	[fspi_eraseSector] = seq_micronEraseSector,
	[fspi_eraseBlock] = seq_micronEraseBlock,
	[fspi_eraseChip] = seq_micronEraseBulk,
	[fspi_programQPP] = seq_micronProgramQPP,
	[fspi_readID] = seq_genericReadID,
};

/* Micron chips with stacked dice >=1Gb */
static const u32 *lutMicronDie[LUT_ENTRIES] = {
	[fspi_readData] = seq_micronReadData,
	[fspi_readStatus] = seq_genericReadStatus,
	[fspi_writeStatus] = seq_genericWriteStatus,
	[fspi_writeEnable] = seq_genericWriteEnable,
	[fspi_writeDisable] = seq_genericWriteDisable,
	[fspi_eraseSector] = seq_micronEraseSector,
	[fspi_eraseBlock] = seq_micronEraseBlock,
	[fspi_eraseChip] = seq_micronEraseDie,
	[fspi_programQPP] = seq_micronProgramQPP,
	[fspi_readID] = seq_genericReadID,
	[fspi_enter4byteAddr] = seq_micronEnter4Byte,
	[fspi_exit4byteAddr] = seq_micronExit4Byte,
};

#endif /* _LUTTABLES_H_ */
