/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * Common Flash Interface for flash drive
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include "flashcfg.h"
#include <lib/errno.h>


#define MANUFACTURER_ID_SPANSION 0x1
#define SPANSION_MEM256          0x19
#define SPANSION_MEM128          0x18


static void flashcfg_spansionCmds(flash_info_t *info)
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


int flashcfg_infoResolve(flash_info_t *info)
{
	int res = EOK;

	if (info->cfi.vendorData[0] == MANUFACTURER_ID_SPANSION && info->cfi.vendorData[2] == SPANSION_MEM256) {
		info->name = "Spansion s25fl256s1";
		flashcfg_spansionCmds(info);
	}
	else {
		info->name = "Unknown";
		res = -EINVAL;
	}

	return res;
}
