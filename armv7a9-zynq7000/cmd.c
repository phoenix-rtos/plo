/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * Loader commands
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../errors.h"
#include "../plostd.h"
#include "../phfs.h"
#include "../low.h"
#include "../cmd.h"
#include "../elf.h"

#include "config.h"
#include "cmd-board.h"


/* TODO: download bitstream via phfs and put it into BISTREAM_ADDR */
void cmd_loadPL(char *args)
{
	// _zynq_loadPL(BISTREAM_ADDR, 0x3dbafc);
}