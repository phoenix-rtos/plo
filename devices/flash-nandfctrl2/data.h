/*
 * Phoenix-RTOS
 *
 * Operating system loader
 *
 * GRLIB NANDFCTRL2 Data Interface
 *
 * Copyright 2026 Phoenix Systems
 * Author: Lukasz Leczkowski
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FLASH_NANDFCTRL2_DATA_H_
#define FLASH_NANDFCTRL2_DATA_H_


#include "nandfctrl2.h"


typedef struct {
	nandfctrl2_dma_t *dma;
	const nandfctrl2_info_t *cfg;
} nand_t;


nand_t *nand_get(unsigned int minor);


int data_doSync(nand_t *nand);

#endif
