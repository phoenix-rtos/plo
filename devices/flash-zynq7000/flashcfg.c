/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Common Flash Interface for flash drive
 *
 * Copyright 2021-2022 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "flashcfg.h"
#include <lib/errno.h>

/* Generic flash commands */

#define FLASH_CMD_RDID      0x9f /* Read JEDEC ID */
#define FLASH_CMD_RDSR1     0x05 /* Read Status Register - 1 */
#define FLASH_CMD_WRDI      0x04 /* Write enable */
#define FLASH_CMD_WREN      0x06 /* Write disable */
#define FLASH_CMD_READ      0x03 /* Read data */
#define FLASH_CMD_FAST_READ 0x0b /* Fast read data */
#define FLASH_CMD_DOR       0x3b /* Dual output fast read data */
#define FLASH_CMD_QOR       0x6b /* Quad output read data */
#define FLASH_CMD_DIOR      0xbb /* Dual input/output fast read data */
#define FLASH_CMD_QIOR      0xeb /* Quad input/output fast read data */
#define FLASH_CMD_PP        0x02 /* Page program */
#define FLASH_CMD_QPP       0x32 /* Quad input fast program */
#define FLASH_CMD_P4E       0x20 /* 4KB sector erase */
#define FLASH_CMD_SE        0xd8 /* 64KB sector erase*/
#define FLASH_CMD_BE        0x60 /* Chip erase */


static void flashcfg_defaultCmds(flash_info_t *info)
{
	info->cmds[flash_cmd_rdid].opCode = FLASH_CMD_RDID;
	info->cmds[flash_cmd_rdid].size = 1;
	info->cmds[flash_cmd_rdid].dummyCyc = 24;

	info->cmds[flash_cmd_rdsr1].opCode = FLASH_CMD_RDSR1;
	info->cmds[flash_cmd_rdsr1].size = 1;
	info->cmds[flash_cmd_rdsr1].dummyCyc = 8;

	info->cmds[flash_cmd_wrdi].opCode = FLASH_CMD_WRDI;
	info->cmds[flash_cmd_wrdi].size = 1;
	info->cmds[flash_cmd_wrdi].dummyCyc = 0;

	info->cmds[flash_cmd_wren].opCode = FLASH_CMD_WREN;
	info->cmds[flash_cmd_wren].size = 1;
	info->cmds[flash_cmd_wren].dummyCyc = 0;

	info->cmds[flash_cmd_read].opCode = FLASH_CMD_READ;
	info->cmds[flash_cmd_read].size = 4;
	info->cmds[flash_cmd_read].dummyCyc = 0;

	info->cmds[flash_cmd_fast_read].opCode = FLASH_CMD_FAST_READ;
	info->cmds[flash_cmd_fast_read].size = 4;
	info->cmds[flash_cmd_fast_read].dummyCyc = 8;

	info->cmds[flash_cmd_dor].opCode = FLASH_CMD_DOR;
	info->cmds[flash_cmd_dor].size = 4;
	info->cmds[flash_cmd_dor].dummyCyc = 8;

	info->cmds[flash_cmd_qor].opCode = FLASH_CMD_QOR;
	info->cmds[flash_cmd_qor].size = 4;
	info->cmds[flash_cmd_qor].dummyCyc = 8;

	info->cmds[flash_cmd_dior].opCode = FLASH_CMD_DIOR;
	info->cmds[flash_cmd_dior].size = 4;
	info->cmds[flash_cmd_dior].dummyCyc = 8;

	info->cmds[flash_cmd_qior].opCode = FLASH_CMD_QIOR;
	info->cmds[flash_cmd_qior].size = 4;
	info->cmds[flash_cmd_qior].dummyCyc = 24;

	info->cmds[flash_cmd_pp].opCode = FLASH_CMD_PP;
	info->cmds[flash_cmd_pp].size = 4;
	info->cmds[flash_cmd_pp].dummyCyc = 0;

	info->cmds[flash_cmd_qpp].opCode = FLASH_CMD_QPP;
	info->cmds[flash_cmd_qpp].size = 4;
	info->cmds[flash_cmd_qpp].dummyCyc = 0;

	info->cmds[flash_cmd_p4e].opCode = FLASH_CMD_P4E;
	info->cmds[flash_cmd_p4e].size = 4;
	info->cmds[flash_cmd_p4e].dummyCyc = 0;

	info->cmds[flash_cmd_p64e].opCode = FLASH_CMD_SE;
	info->cmds[flash_cmd_p64e].size = 4;
	info->cmds[flash_cmd_p64e].dummyCyc = 0;

	info->cmds[flash_cmd_be].opCode = FLASH_CMD_BE;
	info->cmds[flash_cmd_be].size = 1;
	info->cmds[flash_cmd_be].dummyCyc = 0;
}


void flashcfg_jedecIDGet(flash_cmd_t *cmd)
{
	cmd->opCode = FLASH_CMD_RDID;
	cmd->size = 1;
	cmd->dummyCyc = 24;
}


int flashcfg_infoResolve(flash_info_t *info)
{
	int res = EOK;

	/* Spansion s25fl256s1 */
	if (info->cfi.vendorData[0] == 0x1 && info->cfi.vendorData[2] == 0x19) {
		info->name = "Spansion s25fl256s1";
		flashcfg_defaultCmds(info);
	}
	/* Micron n25q128 */
	else if (info->cfi.vendorData[0] == 0x20 && info->cfi.vendorData[1] == 0xbb && info->cfi.vendorData[2] == 0x18) {
		info->name = "Micron n25q128";

		/* Lack of CFI support, filled based on specification */
		info->cfi.timeoutTypical.byteWrite = 0x6;
		info->cfi.timeoutTypical.pageWrite = 0x9;
		info->cfi.timeoutTypical.sectorErase = 0x8;
		info->cfi.timeoutTypical.chipErase = 0xf;
		info->cfi.timeoutMax.byteWrite = 0x2;
		info->cfi.timeoutMax.pageWrite = 0x2;
		info->cfi.timeoutMax.sectorErase = 0x3;
		info->cfi.timeoutMax.chipErase = 0x3;
		info->cfi.chipSize = 0x18;
		info->cfi.fdiDesc = 0x0102;
		info->cfi.pageSize = 0x08;
		info->cfi.regsCount = 1;
		info->cfi.regs[0].count = 0xff;
		info->cfi.regs[0].size = 0x100;

		flashcfg_defaultCmds(info);
		/* Specific configuration */
		info->cmds[flash_cmd_dior].dummyCyc = 16;
		info->cmds[flash_cmd_qior].dummyCyc = 32;
	}
	/* Winbond W25Q128JV - IM/JM */
	else if (info->cfi.vendorData[0] == 0xef && info->cfi.vendorData[1] == 0x40 && info->cfi.vendorData[2] == 0x18) {
		info->name = "Winbond W25Q128JV";

		/* Lack of CFI support, filled based on specification */
		info->cfi.timeoutTypical.byteWrite = 0x6;
		info->cfi.timeoutTypical.pageWrite = 0x9;
		info->cfi.timeoutTypical.sectorErase = 0x8;
		info->cfi.timeoutTypical.chipErase = 0xf;
		info->cfi.timeoutMax.byteWrite = 0x2;
		info->cfi.timeoutMax.pageWrite = 0x2;
		info->cfi.timeoutMax.sectorErase = 0x3;
		info->cfi.timeoutMax.chipErase = 0x3;
		info->cfi.chipSize = 0x18;
		info->cfi.fdiDesc = 0x0102;
		info->cfi.pageSize = 0x08;
		info->cfi.regsCount = 1;
		info->cfi.regs[0].count = 0xff;
		info->cfi.regs[0].size = 0x100;

		flashcfg_defaultCmds(info);
	}
	else {
		info->name = "Unknown";
		res = -EINVAL;
	}

	return res;
}
