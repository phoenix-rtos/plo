/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * i.MX RT hyperflash NOR device driver
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Gerard Swiderski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _HYPER_LUT_H_
#define _HYPER_LUT_H_


#define LUTSZ_HYPER (4 * 12)

/* Spansion 6KS512SDPHIO2 */

const u32 hyperLut[LUTSZ_HYPER] = {
	/* 0: Read Data */
	[4 * fspi_hyperReadData + 0] = LUT_SEQ(lutCmd_DDR, lutPad8, 0xa0, lutCmdRADDR_DDR, lutPad8, 0x18),
	[4 * fspi_hyperReadData + 1] = LUT_SEQ(lutCmdCADDR_DDR, lutPad8, 0x10, lutCmdREAD_DDR, lutPad8, 0x04),
	[4 * fspi_hyperReadData + 2] = LUT_SEQ(lutCmdSTOP, lutPad1, 0x00, 0, 0, 0),
	[4 * fspi_hyperReadData + 3] = 0,

	/* 1: Write Data */
	[4 * fspi_hyperWriteData + 0] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x20, lutCmdRADDR_DDR, lutPad8, 0x18),
	[4 * fspi_hyperWriteData + 1] = LUT_SEQ(lutCmdCADDR_DDR, lutPad8, 0x10, lutCmdWRITE_DDR, lutPad8, 0x02),
	[4 * fspi_hyperWriteData + 2] = LUT_SEQ(lutCmdSTOP, lutPad1, 0x00, 0, 0, 0),
	[4 * fspi_hyperWriteData + 3] = 0,

	/* 2: Read Status */
	[4 * fspi_hyperReadStatus + 0] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x00),
	[4 * fspi_hyperReadStatus + 1] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0xaa), /* address: 0x555 */
	[4 * fspi_hyperReadStatus + 2] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x05),
	[4 * fspi_hyperReadStatus + 3] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x70), /* data: 0x70 */

	[4 * fspi_hyperReadStatus + 4] = LUT_SEQ(lutCmd_DDR, lutPad8, 0xa0, lutCmdRADDR_DDR, lutPad8, 0x18),
	[4 * fspi_hyperReadStatus + 5] = LUT_SEQ(lutCmdCADDR_DDR, lutPad8, 0x10, lutCmdDUMMY_RWDS_DDR, lutPad8, 0x0b),
	[4 * fspi_hyperReadStatus + 6] = LUT_SEQ(lutCmdREAD_DDR, lutPad8, 0x04, lutCmdSTOP, lutPad1, 0x0),
	[4 * fspi_hyperReadStatus + 7] = 0,

	/* 4: Write Enable */
	[4 * fspi_hyperWriteEnable + 0] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x00),
	[4 * fspi_hyperWriteEnable + 1] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0xaa), /* address: 0x555 */
	[4 * fspi_hyperWriteEnable + 2] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x05),
	[4 * fspi_hyperWriteEnable + 3] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0xaa), /* data: 0xaa */

	[4 * fspi_hyperWriteEnable + 4] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x00),
	[4 * fspi_hyperWriteEnable + 5] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x55),
	[4 * fspi_hyperWriteEnable + 6] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x02),
	[4 * fspi_hyperWriteEnable + 7] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x55),

	/* 6: Erase Sector  */
	[4 * fspi_hyperEraseSector + 0] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x00),
	[4 * fspi_hyperEraseSector + 1] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0xaa), /* address: 0x555 */
	[4 * fspi_hyperEraseSector + 2] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x05),
	[4 * fspi_hyperEraseSector + 3] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x80), /* data: 0x80 */

	[4 * fspi_hyperEraseSector + 4] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x00),
	[4 * fspi_hyperEraseSector + 5] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0xaa),
	[4 * fspi_hyperEraseSector + 6] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x05),
	[4 * fspi_hyperEraseSector + 7] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0xaa), /* address: 0x555 */

	[4 * fspi_hyperEraseSector + 8] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x00),
	[4 * fspi_hyperEraseSector + 9] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x55),
	[4 * fspi_hyperEraseSector + 10] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x02),
	[4 * fspi_hyperEraseSector + 11] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x55),

	[4 * fspi_hyperEraseSector + 12] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmdRADDR_DDR, lutPad8, 0x18),
	[4 * fspi_hyperEraseSector + 13] = LUT_SEQ(lutCmdCADDR_DDR, lutPad8, 0x10, lutCmd_DDR, lutPad8, 0x00),
	[4 * fspi_hyperEraseSector + 14] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x30, lutCmdSTOP, lutPad1, 0x00),
	[4 * fspi_hyperEraseSector + 15] = LUT_SEQ(lutCmdSTOP, lutPad1, 0x00, 0, 0, 0),

	/* 10: Page Program */
	[4 * fspi_hyperPageProgram + 0] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x00),
	[4 * fspi_hyperPageProgram + 1] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0xaa), /* address: 0x555 */
	[4 * fspi_hyperPageProgram + 2] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0x05),
	[4 * fspi_hyperPageProgram + 3] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmd_DDR, lutPad8, 0xa0), /* DATA 0xa0 */

	[4 * fspi_hyperPageProgram + 4] = LUT_SEQ(lutCmd_DDR, lutPad8, 0x00, lutCmdRADDR_DDR, lutPad8, 0x18),
	[4 * fspi_hyperPageProgram + 5] = LUT_SEQ(lutCmdCADDR_DDR, lutPad8, 0x10, lutCmdWRITE_DDR, lutPad8, 0x80),
	[4 * fspi_hyperPageProgram + 6] = LUT_SEQ(lutCmdSTOP, lutPad1, 0x00, 0, 0, 0),
	[4 * fspi_hyperPageProgram + 7] = 0,
};


#endif /* _HYPER_LUT_H_ */
